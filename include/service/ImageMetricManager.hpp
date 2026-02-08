#pragma once

#include "Volume.hpp"
#include "command/image_metrics/TriggerCapture.hpp"
#include "service/detail/image_metric_manager/CopyDeviceToHostMemory.hpp"
#include "service/detail/image_metric_manager/HostVisibleStorage.hpp"

#include <starlight/core/device/managers/GraphicsContainer.hpp>
#include <starlight/policy/command/ListenFor.hpp>

template <typename T>
using ListenForTriggerCapture =
    star::policy::command::ListenFor<T, image_metrics::TriggerCapture,
                                     image_metrics::trigger_capture::GetTriggerCaptureCommandTypeName, &T::onCapture>;

/// <summary>
/// Responsible for gathering all needed information from shaders and compute operations needed for
/// image label processing
/// </summary>
///
class ImageMetricManager
{
  public:
    ImageMetricManager();
    ImageMetricManager(const ImageMetricManager &) = delete;
    ImageMetricManager &operator=(const ImageMetricManager &) = delete;
    ImageMetricManager(ImageMetricManager &&other);
    ImageMetricManager &operator=(ImageMetricManager &&other);

    void init();

    void setInitParameters(star::service::InitParameters &params);

    void negotiateWorkers(star::core::WorkerPool &pool, star::job::TaskManager &tm)
    {
    }

    void shutdown();

    void recordThisFrame(const Volume &volume, const std::string &imageCaptureFileName);

    void onCapture(image_metrics::TriggerCapture &cmd);

  private:
    image_metric_manager::HostVisibleStorage m_storage;
    image_metric_manager::CopyDeviceToHostMemory m_copier;
    ListenForTriggerCapture<ImageMetricManager> m_listenerCapture;
    star::core::CommandSubmitter m_cmdSubmitter;
    star::core::CommandSubmitter m_cmdSubmitterUpdater;
    star::core::CommandSubmitter m_cmdSubmitterTrigger;
    star::core::CommandBus *m_cmdBus = nullptr;
    star::core::device::StarDevice *m_device = nullptr;
    star::common::EventBus *m_eb = nullptr;
    star::core::device::manager::ManagerCommandBuffer *m_cb = nullptr;
    star::core::device::manager::Queue *m_qm = nullptr;
    star::core::device::manager::Semaphore *m_s = nullptr;
    const star::common::FrameTracker *m_frameTracker = nullptr;
    bool m_isRegistered = false; 

    void initCopier(star::core::device::DeviceContext &context);

    void initListeners(star::core::CommandBus &cmdBus);

    void cleanupListeners(star::core::CommandBus &cmdBus);
};