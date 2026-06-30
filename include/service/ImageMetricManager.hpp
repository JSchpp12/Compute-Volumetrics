#pragma once

#include "Volume.hpp"

#include "command/image_metrics/GetTransferCopyPass.hpp"
#include "command/image_metrics/RegisterTerrainRecordInfo.hpp"
#include "command/image_metrics/RegisterVolumeRecordInfo.hpp"
#include "command/image_metrics/TriggerCapture.hpp"
#include "service/detail/image_metric_manager/CopyDeviceToHostMemory.hpp"
#include "service/detail/image_metric_manager/HostVisibleStorage.hpp"
#include "service/detail/image_metric_manager/SharedBufferHandle.hpp"

#include <star_terrain/file_data/CoverageInfo.hpp>
#include <starlight/command/TaskScheduler/SubmitTask.hpp>
#include <starlight/core/device/managers/GraphicsContainer.hpp>
#include <starlight/job/tasks/Task.hpp>
#include <starlight/policy/command/ListenFor.hpp>
#include <starlight/service/TaskSchedulerService.hpp>

#include <future>
#include <memory>
#include <optional>

namespace service
{
template <typename T>
using ListenForTriggerCapture =
    star::policy::command::ListenFor<T, image_metrics::TriggerCapture,
                                     image_metrics::trigger_capture::GetTriggerCaptureCommandTypeName, &T::onCapture>;

template <typename T>
using ListenForRegisterTerrainData =
    star::policy::command::ListenFor<T, image_metrics::RegisterTerrainRecordInfo,
                                     image_metrics::register_terrain_record_info::GetUniqueName,
                                     &T::onRegisterTerrainRecord>;

template <typename T>
using ListenForRegisterVolumeData =
    star::policy::command::ListenFor<T, image_metrics::RegisterVolumeRecordInfo,
                                     image_metrics::register_volume_record_info::GetUniqueName,
                                     &T::onRegisterVolumeRecord>;

template <typename T>
using ListenForGetTransferCopyPass =
    star::policy::command::ListenFor<T, image_metrics::GetTransferCopyPass,
                                     image_metrics::get_transfer_copy_pass::GetTransferCopyPassCommandTypeName,
                                     &T::onGetTransferCopyPass>;

/// <summary>
/// Responsible for gathering all needed information from shaders and compute operations needed for
/// image label processing
/// </summary>
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

    void recordThisFrame(const star::Light &mainLight, const Volume &volume, const std::string &imageCaptureFileName,
                         const star::StarCamera &camera);

    void onCapture(image_metrics::TriggerCapture &cmd);

    void onGetTransferCopyPass(image_metrics::GetTransferCopyPass &cmd);

    void onRegisterTerrainRecord(image_metrics::RegisterTerrainRecordInfo &cmd);

    void onRegisterVolumeRecord(image_metrics::RegisterVolumeRecordInfo &cmd);

  private:
    class LoadingShapeInfo
    {
        std::optional<star::terrain::CoverageInfo> m_cachedTerrainShapeInfo{std::nullopt};
        std::future<star::terrain::CoverageInfo> m_inProgressLoadingShapeInfo;
        std::string m_terrainName;
        star::terrain::rendering::Type m_terrainRenderingType;

      public:
        LoadingShapeInfo() = default;
        explicit LoadingShapeInfo(std::future<star::terrain::CoverageInfo> future, std::string terrainName,
                                  star::terrain::rendering::Type terrainRenderingType)
            : m_cachedTerrainShapeInfo{std::nullopt}, m_inProgressLoadingShapeInfo(std::move(future)),
              m_terrainName(std::move(terrainName)), m_terrainRenderingType(terrainRenderingType)
        {
        }

        star::terrain::CoverageInfo &get() noexcept
        {
            if (m_cachedTerrainShapeInfo.has_value())
                return m_cachedTerrainShapeInfo.value();

            m_cachedTerrainShapeInfo = m_inProgressLoadingShapeInfo.get();
            return m_cachedTerrainShapeInfo.value();
        }
        const std::string &getTerrainName() const noexcept
        {
            return m_terrainName;
        }
        star::terrain::rendering::Type getTerrainRenderingType() const noexcept
        {
            return m_terrainRenderingType;
        }
    };

    image_metric_manager::HostVisibleStorage m_storage;
    LoadingShapeInfo m_cachedTerrainShapeInfo;
    std::string m_cachedVolumeNameInfo;
    image_metric_manager::CopyDeviceToHostMemory m_copier;
    star::core::CommandBus *m_cmdBus = nullptr;
    star::core::device::StarDevice *m_device = nullptr;
    star::common::EventBus *m_eb = nullptr;
    star::core::device::manager::ManagerCommandBuffer *m_cb = nullptr;
    star::core::device::manager::Queue *m_qm = nullptr;
    star::core::device::manager::Semaphore *m_s = nullptr;
    const star::common::FrameTracker *m_frameTracker = nullptr;
    star::service::TaskSchedulerService *m_taskScheduler = nullptr;
    bool m_isRegistered = false;
    ListenForTriggerCapture<ImageMetricManager> m_listenerCapture;
    ListenForRegisterTerrainData<ImageMetricManager> m_listenerTerrainInfo;
    ListenForRegisterVolumeData<ImageMetricManager> m_listenerVolumeInfo;
    ListenForGetTransferCopyPass<ImageMetricManager> m_listenerGetTransferCopyPass;

    void initListeners(star::core::CommandBus &cmdBus);
    void cleanupListeners(star::core::CommandBus &cmdBus);
    void submitToGatherTerrainInfoFromFile(std::filesystem::path terrainShapeFilePath,
                                           star::terrain::rendering::Type renderingType);
};
} // namespace service