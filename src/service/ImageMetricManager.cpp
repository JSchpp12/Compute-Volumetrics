#include "service/ImageMetricManager.hpp"

#include "TerrainShapeInfoLoader.hpp"
#include "service/detail/image_metric_manager/FileWriteFunction.hpp"
#include "service/detail/image_metric_manager/RayMaskFiles.hpp"
#include "service/detail/image_metric_manager/SharedBufferWriteImagePayload.hpp"
#include "service/detail/image_metric_manager/SharedBufferWriteValidityMaskPayload.hpp"

#include <starlight/command/FileIO/WriteToFile.hpp>
#include <starlight/command/GetScreenCaptureSyncInfo.hpp>
#include <starlight/command/TaskScheduler/SubmitTask.hpp>
#include <starlight/command/command_order/DeclareDependency.hpp>
#include <starlight/command/frames/GetFrameTracker.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/job/tasks/WriteImageToDisk.hpp>

namespace service
{
ImageMetricManager::ImageMetricManager()
    : m_storage(), m_cachedTerrainShapeInfo(), m_cachedVolumeNameInfo(), m_copier(), m_cmdBus(nullptr),
      m_device(nullptr), m_eb(nullptr), m_cb(nullptr), m_qm(nullptr), m_s(nullptr), m_frameTracker(nullptr),
      m_taskScheduler(nullptr), m_isRegistered(false), m_listenerCapture(*this), m_listenerTerrainInfo(*this),
      m_listenerVolumeInfo(*this)
{
}

ImageMetricManager::ImageMetricManager(ImageMetricManager &&other)
    : m_storage(std::move(other.m_storage)), m_cachedTerrainShapeInfo(std::move(other.m_cachedTerrainShapeInfo)),
      m_cachedVolumeNameInfo(std::move(other.m_cachedVolumeNameInfo)), m_copier(), m_cmdBus(other.m_cmdBus),
      m_device(other.m_device), m_eb(other.m_eb), m_cb(other.m_cb), m_qm(other.m_qm), m_s(other.m_s),
      m_frameTracker(other.m_frameTracker), m_taskScheduler(other.m_taskScheduler), m_listenerCapture(*this),
      m_listenerTerrainInfo(*this), m_listenerVolumeInfo(*this)
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
        m_taskScheduler = other.m_taskScheduler;

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
    m_taskScheduler = params.taskScheduler;

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

void ImageMetricManager::onRegisterVolumeRecord(image_metrics::RegisterVolumeRecordInfo &cmd)
{
    m_cachedVolumeNameInfo = cmd.volumeName;
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

    const size_t fi = static_cast<size_t>(m_frameTracker->getCurrent().getFrameInFlightIndex());
    star::Handle hostResource;

    vk::Extent2D iResolution;
    {
        const auto &resolution = volume.getRenderer().getRenderToImages().at(fi)->getBaseExtent();
        iResolution.setHeight(resolution.height).setWidth(resolution.width);
        if (!m_storage.contains(iResolution))
        {
            m_storage.createResourcePoolForResolution(iResolution, *m_frameTracker, *m_device, *m_eb);
        }
        hostResource = m_storage.getAvailableResource(iResolution);
    }

    const uint64_t signalValue = m_frameTracker->getCurrent().getNumTimesFrameProcessed() + 1;

    assert(m_s != nullptr);
    const star::Handle &semaphore = m_storage.getCopyDoneSemaphores()[fi];
    star::core::device::manager::SemaphoreRecord *semaphoreRecord = m_s->get(semaphore);

    assert(m_cb != nullptr);
    auto resources = m_storage.getResource(hostResource);
    m_copier.setCopyDst(signalValue, resources, semaphore, semaphoreRecord);
    m_copier.setCopySrc({.registration = volume.getRenderer().getCommandBuffer(),
                         .rayDistance = volume.getRenderer().getRayDistanceBufferAt(fi),
                         .rayAtCutoffDist = volume.getRenderer().getRayAtCutoffBufferAt(fi)});
    m_copier.trigger(*m_cb, *m_cmdBus);

    auto sharedHandle = std::make_shared<image_metric_manager::SharedBufferHandle>(
        &m_storage, hostResource, resources, iResolution, m_device->getVulkanDevice(), semaphoreRecord->semaphore,
        signalValue);

    const auto basePath = std::filesystem::path(imageCaptureFileName);
    const auto maskPath = (basePath.parent_path() / (basePath.stem().string() + "_distanceMask.tif")).string();
    const auto normalizedMaskPath =
        (basePath.parent_path() / (basePath.stem().string() + "_distanceMaskNorm.tif")).string();
    const auto jsonPath = std::filesystem::path(imageCaptureFileName).replace_extension(".json").string();
    const auto rayValidityMaskPath = (basePath.parent_path() / (basePath.stem().string() + "_validMask.png")).string();

    {
        auto tifPayload =
            star::job::tasks::write_image_to_disk::Create(image_metric_manager::SharedBufferWriteImagePayload{
                .bufferHandle = sharedHandle, .imageFormat = vk::Format::eR32Sfloat, .path = maskPath});

        star::command::task_scheduler::SubmitTask tifCmd(std::move(tifPayload),
                                                         star::job::tasks::write_image_to_disk::WriteImageTypeName);
        m_cmdBus->submit(tifCmd);
    }

    {
        auto normalizedTifPayload =
            star::job::tasks::write_image_to_disk::Create(image_metric_manager::SharedBufferWriteImagePayload{
                .bufferHandle = sharedHandle,
                .imageFormat = vk::Format::eR32Sfloat,
                .path = normalizedMaskPath,
                .normalizeFloatRanges = true,
                .applyCompression = false,
            });

        star::command::task_scheduler::SubmitTask tifCmd(std::move(normalizedTifPayload),
                                                         star::job::tasks::write_image_to_disk::WriteImageTypeName);
        m_cmdBus->submit(tifCmd);
    }

    {
        auto rayValidityPayload =
            star::job::tasks::write_image_to_disk::Create(image_metric_manager::SharedBufferWriteValidityMaskPayload{
                .bufferHandle = sharedHandle, .imageFormat = vk::Format::eR8Uint, .path = rayValidityMaskPath});

        star::command::task_scheduler::SubmitTask validityCmd(
            std::move(rayValidityPayload), star::job::tasks::write_image_to_disk::WriteImageTypeName);
        m_cmdBus->submit(validityCmd);
    }

    {
        auto jsonPayload = star::job::tasks::io::CreateWriteTask(star::job::tasks::io::WritePayload{
            jsonPath,
            image_metric_manager::FileWriteFunction{
                sharedHandle, iResolution, camera, volume, mainLight, m_cachedTerrainShapeInfo.getTerrainName(),
                m_cachedTerrainShapeInfo.get(), m_cachedTerrainShapeInfo.getTerrainRenderingType(),
                m_cachedVolumeNameInfo, imageCaptureFileName,
                image_metric_manager::RayMaskFiles{rayValidityMaskPath, maskPath, normalizedMaskPath}}});

        star::command::file_io::WriteToFile jsonCmd(std::move(jsonPayload));
        m_cmdBus->submit(jsonCmd);
    }
}

void ImageMetricManager::initListeners(star::core::CommandBus &cmdBus)
{
    m_listenerCapture.init(cmdBus);
    m_listenerTerrainInfo.init(cmdBus);
    m_listenerVolumeInfo.init(cmdBus);
}

void ImageMetricManager::cleanupListeners(star::core::CommandBus &cmdBus)
{
    m_listenerCapture.cleanup(cmdBus);
    m_listenerTerrainInfo.cleanup(cmdBus);
    m_listenerVolumeInfo.cleanup(cmdBus);
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