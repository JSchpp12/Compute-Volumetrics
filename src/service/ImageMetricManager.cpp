#include "service/ImageMetricManager.hpp"

#include "TerrainShapeInfoLoader.hpp"
#include "service/detail/image_metric_manager/FileWriteFunction.hpp"

#include <starlight/command/FileIO/WriteToFile.hpp>
#include <starlight/command/GetScreenCaptureSyncInfo.hpp>
#include <starlight/command/command_order/DeclareDependency.hpp>
#include <starlight/command/frames/GetFrameTracker.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

namespace service
{
ImageMetricManager::ImageMetricManager()
    : m_storage(), m_copier(), m_listenerCapture(*this), m_listenerTerrainInfo(*this)
{
}

ImageMetricManager::ImageMetricManager(ImageMetricManager &&other)
    : m_storage(std::move(other.m_storage)), m_copier(), m_listenerCapture(*this), m_listenerTerrainInfo(*this),
      m_cmdBus(other.m_cmdBus), m_device(other.m_device), m_eb(other.m_eb), m_cb(other.m_cb), m_qm(other.m_qm),
      m_s(other.m_s), m_frameTracker(other.m_frameTracker)
{
    if (m_cmdBus != nullptr)
    {
        other.cleanupListeners(*m_cmdBus);
        initListeners(*m_cmdBus);
    }
}

ImageMetricManager &ImageMetricManager::operator=(ImageMetricManager &&other)
{
    if (this != &other)
    {
        m_storage = std::move(other.m_storage);
        m_cmdBus = other.m_cmdBus;
        m_device = other.m_device;
        m_eb = other.m_eb;
        m_cb = other.m_cb;
        m_qm = other.m_qm;
        m_s = other.m_s;
        m_frameTracker = other.m_frameTracker;

        // going to ignore the copier for now...
        // it should be init once the manager has been put in final place

        if (m_cmdBus != nullptr)
        {
            other.cleanupListeners(*m_cmdBus);
            initListeners(*m_cmdBus);
        }
    }

    return *this;
}

void ImageMetricManager::setInitParameters(star::service::InitParameters &params)
{
    m_cmdBus = &params.commandBus;
    m_device = &params.device;
    m_eb = &params.eventBus;
    m_cb = &params.commandBufferManager;
    m_qm = &params.graphicsManagers.queueManager;
    m_s = params.graphicsManagers.semaphoreManager.get();

    star::frames::GetFrameTracker ftCmd{};
    params.commandBus.submit(ftCmd);
    assert(ftCmd.getReply().get() != nullptr);

    m_frameTracker = ftCmd.getReply().get();
}

void ImageMetricManager::shutdown()
{
    m_storage.cleanupRender();

    cleanupListeners(*m_cmdBus);
}

void ImageMetricManager::init()
{
    assert(m_cb != nullptr);

    m_copier.prepRender(*m_device, *m_eb, *m_cmdBus, *m_cb, *m_qm, *m_frameTracker);
    m_storage.prepRender(*m_frameTracker, *m_eb);

    initListeners(*m_cmdBus);
}

void ImageMetricManager::onCapture(image_metrics::TriggerCapture &cmd)
{
    recordThisFrame(cmd.mainLight, cmd.volumeObject, cmd.srcImagePath, cmd.camera);
}

void ImageMetricManager::onRegisterTerrainRecord(image_metrics::RegisterTerrainRecordInfo &cmd)
{
    submitToGatherTerrainInfoFromFile(cmd.terrainHeightFilePath, cmd.terrainRenderingType);
}

void ImageMetricManager::recordThisFrame(const star::Light &mainLight, const Volume &volume,
                                         const std::string &imageCaptureFileName, const star::StarCamera &camera)
{
    if (!m_isRegistered)
    {
        const auto type = m_cmdBus->registerCommandType(
            star::command_order::declare_dependency::GetDeclareDependencyCommandTypeName());

        auto dCmd = star::command_order::DeclareDependency(type, volume.getRenderer().getCommandBuffer(),
                                                           m_copier.getCommandBuffer());
        m_cmdBus->submit(dCmd);

        m_isRegistered = true;
    }

    // select resource to use
    const size_t fi = static_cast<size_t>(m_frameTracker->getCurrent().getFrameInFlightIndex());
    star::Handle hostResource;
    {
        const auto &resolution = volume.getRenderer().getRenderToImages().at(fi)->getBaseExtent();
        const auto iResolution = vk::Extent2D().setHeight(resolution.height).setWidth(resolution.width);

        if (!m_storage.contains(iResolution))
        {
            m_storage.createResourcePoolForResolution(iResolution, *m_frameTracker, *m_device, *m_eb);
        }
        hostResource = m_storage.getAvailableBufferToUse(iResolution);
    }

    const uint64_t signalValue = m_frameTracker->getCurrent().getNumTimesFrameProcessed() + 1;

    assert(m_s != nullptr);
    const star::Handle &semaphore = m_storage.getCopyDoneSemaphores()[fi];
    star::core::device::manager::SemaphoreRecord *semaphoreRecord = m_s->get(semaphore);

    assert(m_cb != nullptr);
    const star::StarBuffers::Buffer *rayDistance = nullptr;
    const star::StarBuffers::Buffer *rayAtCutoff = nullptr;
    m_storage.getRayDistanceBuffers(hostResource, &rayDistance, &rayAtCutoff);

    m_copier.trigger(*m_cb, *m_cmdBus, *rayAtCutoff, *rayDistance, volume.getRenderer().getRayAtCutoffBufferAt(fi),
                     volume.getRenderer().getRayDistanceBufferAt(fi), semaphore, semaphoreRecord, signalValue,
                     volume.getRenderer().getCommandBuffer());

    star::core::logging::info("Submitting write task for file: " +
                              std::filesystem::path(imageCaptureFileName).replace_extension(".json").string());
    {
        auto writePayload = star::job::tasks::io::CreateWriteTask(star::job::tasks::io::WritePayload{
            imageCaptureFileName,
            service::image_metric_manager::FileWriteFunction{
                mainLight, volume.getRenderer().getFogInfo(), camera.getPosition(), camera.getForwardVector(),
                hostResource, m_device->getVulkanDevice(), semaphoreRecord->semaphore, signalValue,
                volume.getRenderer().getFogType(), &m_storage, m_cachedTerrainShapeInfo.getTerrainName(),
                m_cachedTerrainShapeInfo.get(), m_cachedTerrainShapeInfo.getTerrainRenderingType()}});
        star::command::file_io::WriteToFile writeCmd{std::move(writePayload)};
        m_cmdBus->submit(writeCmd);
    }
}

void ImageMetricManager::initListeners(star::core::CommandBus &cmdBus)
{
    m_listenerCapture.init(cmdBus);
    m_listenerTerrainInfo.init(cmdBus);
}

void ImageMetricManager::cleanupListeners(star::core::CommandBus &cmdBus)
{
    m_listenerCapture.cleanup(cmdBus);
    m_listenerTerrainInfo.cleanup(cmdBus);
}

void ImageMetricManager::submitToGatherTerrainInfoFromFile(std::filesystem::path terrainShapeFilePath,
                                                           TerrainRenderingType renderingType)
{
    assert(m_cmdBus != nullptr);
    std::string terrainName = terrainShapeFilePath.parent_path().filename().string();

    m_cachedTerrainShapeInfo =
        LoadingShapeInfo(TerrainShapeInfoLoader::SubmitForRead(std::move(terrainShapeFilePath), *m_cmdBus),
                         std::move(terrainName), renderingType);
}
} // namespace service