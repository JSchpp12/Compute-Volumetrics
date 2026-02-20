#include "service/ImageMetricManager.hpp"

#include "service/detail/image_metric_manager/FileWriteFunction.hpp"

#include <starlight/command/GetScreenCaptureSyncInfo.hpp>
#include <starlight/command/FileIO/WriteToFile.hpp>
#include <starlight/command/command_order/DeclareDependency.hpp>
#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/command/command_order/TriggerPass.hpp>

ImageMetricManager::ImageMetricManager() : m_storage(), m_copier(), m_listenerCapture(*this)
{
}

ImageMetricManager::ImageMetricManager(ImageMetricManager &&other)
    : m_storage(std::move(other.m_storage)), m_copier(), m_listenerCapture(*this), m_cmdSubmitter(other.m_cmdSubmitter),
      m_cmdSubmitterUpdater(std::move(other.m_cmdSubmitterUpdater)),
      m_cmdSubmitterTrigger(std::move(other.m_cmdSubmitterTrigger)), m_cmdBus(other.m_cmdBus), m_device(other.m_device),
      m_eb(other.m_eb), m_cb(other.m_cb), m_qm(other.m_qm), m_s(other.m_s), m_frameTracker(other.m_frameTracker)
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
        m_cmdSubmitter = other.m_cmdSubmitter;
        m_cmdSubmitterUpdater = other.m_cmdSubmitterUpdater;
        m_cmdSubmitterTrigger = other.m_cmdSubmitterTrigger;
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
    m_cmdSubmitter = params.cmdSubmitter;
    m_cmdSubmitterUpdater = params.cmdSubmitter;
    m_cmdSubmitterTrigger = params.cmdSubmitter;
    m_frameTracker = &params.flightTracker;
    m_cmdBus = &params.commandBus;
    m_device = &params.device;
    m_eb = &params.eventBus;
    m_cb = &params.commandBufferManager;
    m_qm = &params.graphicsManagers.queueManager;
    m_s = params.graphicsManagers.semaphoreManager.get();
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

    m_cmdSubmitterTrigger.setType(star::command_order::trigger_pass::GetTriggerPassCommandTypeName());
    m_cmdSubmitter.setType(star::command::file_io::write_to_file::GetWriteToFileCommandTypeName);
    m_cmdSubmitterUpdater.setType(star::command::get_sync_info::GetSyncInfoCommandTypeName);
}

void ImageMetricManager::onCapture(image_metrics::TriggerCapture &cmd)
{
    recordThisFrame(cmd.volumeObject, cmd.srcImagePath);
}

void ImageMetricManager::recordThisFrame(const Volume &volume, const std::string &imageCaptureFileName)
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
                     volume.getRenderer().getRayDistanceBufferAt(fi), semaphoreRecord, signalValue);

    auto writeToFile = image_metric_manager::FileWriteFunction(
        volume.getRenderer().getFogInfo(), hostResource, m_device->getVulkanDevice(), semaphoreRecord->semaphore,
        signalValue, volume.getRenderer().getFogType(), &m_storage);
    auto function = [writeToFile](const std::string &filePath) -> void { writeToFile.write(filePath); };
    {
        auto writeCmd =
            star::command::file_io::WriteToFile::Builder().setFile(imageCaptureFileName).setWriteFileFunction(function).build();
        m_cmdSubmitter.update(writeCmd).submit();
    }

    // properly sync screen capture service cmds -- is done because multiple queues cannot wait on a single binary
    // semaphore so instead of passing forward the binary semaphore from the graphics pass, pass forward a timeline
    // semaphore
    {
        auto getSyncCmd = star::command::GetScreenCaptureSyncInfo();
        m_cmdSubmitterUpdater.update(getSyncCmd).submit();

        if (getSyncCmd.getReply().get().cmdBuffer == nullptr)
        {
            STAR_THROW("Failed to acquire cmd buffer from screen capture service");
        }

        m_cb->get(*getSyncCmd.getReply().get().cmdBuffer)
            .oneTimeWaitSemaphoreInfo.insert(m_copier.getCommandBuffer(), semaphoreRecord->semaphore,
                                             vk::PipelineStageFlagBits::eTransfer, signalValue);
    }
}

void ImageMetricManager::initListeners(star::core::CommandBus &cmdBus)
{
    m_listenerCapture.init(cmdBus);
}

void ImageMetricManager::cleanupListeners(star::core::CommandBus &cmdBus)
{
    m_listenerCapture.cleanup(cmdBus);
}