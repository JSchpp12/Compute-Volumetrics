#pragma once

#include "service/detail/image_metric_manager/CopyCmds.hpp"

#include <starlight/core/device/DeviceContext.hpp>

namespace image_metric_manager
{

/// <summary>
/// Manager for resources necessary for copy + command buffers and their execution
/// </summary>
class CopyDeviceToHostMemory
{
  public:
    CopyDeviceToHostMemory() : m_cpyCmds(m_resources)
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

    void trigger(star::core::device::manager::ManagerCommandBuffer &bufferManager, star::core::CommandBus &cmdBus,
                 const star::StarBuffers::Buffer &hostRayCutoff, const star::StarBuffers::Buffer &hostRayDist,
                 const star::StarBuffers::Buffer &targetRayCutoffBuffer,
                 const star::StarBuffers::Buffer &targetRayDistanceBuffer,
                 star::core::device::manager::SemaphoreRecord *timeline, const uint64_t &valueToSignal);

    const star::Handle &getCommandBuffer()
    {
        return m_cpyCmds.getCommandBuffer();
    }

  private:
    CopyResources m_resources;
    CopyCmds m_cpyCmds;
};
} // namespace image_metric_manager