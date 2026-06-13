#pragma once

#include "service/detail/image_metric_manager/CopyCmds.hpp"
#include "service/detail/image_metric_manager/CopyDstResources.hpp"
#include "service/detail/image_metric_manager/CopySrcResources.hpp"

#include <starlight/core/device/DeviceContext.hpp>

namespace service::image_metric_manager
{

/// <summary>
/// Manager for resources necessary for copy + command buffers and their execution
/// </summary>
class CopyDeviceToHostMemory
{
  public:
    CopyDeviceToHostMemory() : m_srcResources(), m_dstResources(), m_cpyCmds(m_srcResources, m_dstResources)
    {
    }
    CopyDeviceToHostMemory(const CopyDeviceToHostMemory &&) = delete;
    CopyDeviceToHostMemory &operator=(const CopyDeviceToHostMemory &&) = delete;
    CopyDeviceToHostMemory(CopyDeviceToHostMemory &&other) = delete;
    CopyDeviceToHostMemory &operator=(CopyDeviceToHostMemory &&other) = delete;
    ~CopyDeviceToHostMemory() = default;

    void prepRender(star::core::device::StarDevice &device, star::common::EventBus &eb, star::core::CommandBus &cmdBus,
                    star::core::device::manager::ManagerCommandBuffer &cb, star::core::device::manager::Queue &qm,
                    const star::common::FrameTracker &frameTracker);

    void trigger(star::core::device::manager::ManagerCommandBuffer &bufferManager, star::core::CommandBus &cmdBus);

    void setCopySrc(CopySrcResources cpyResource)
    {
        m_srcResources = std::move(cpyResource);
    }

    void setCopyDst(uint64_t signalValue, CopyDstResources cpyResource, star::Handle timelineRecordHandle,
                    star::core::device::manager::SemaphoreRecord *timelineRecord)
    {
        m_dstResources.signalValue = std::move(signalValue);
        m_dstResources.cpyDst = std::move(cpyResource);
        m_dstResources.timelineRecordHandle = std::move(timelineRecordHandle);
        m_dstResources.timelineRecord = timelineRecord;
    }

    const star::Handle &getCommandBuffer()
    {
        return m_cpyCmds.getCommandBuffer();
    }

  private:
    CopySrcResources m_srcResources;
    SynchronizedCopyResourcesInfo m_dstResources;
    CopyCmds m_cpyCmds;
};
} // namespace service::image_metric_manager