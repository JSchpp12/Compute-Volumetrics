#include "service/detail/image_metric_manager/CopyDeviceToHostMemory.hpp"

namespace image_metric_manager
{

void CopyDeviceToHostMemory::prepRender(star::core::device::StarDevice &device, star::common::EventBus &eb,
                                        star::core::CommandBus &cmdBus,
                                        star::core::device::manager::ManagerCommandBuffer &cb,
                                        star::core::device::manager::Queue &qm,
                                        const star::common::FrameTracker &frameTracker)
{
    m_cpyCmds.prepRender(device, cmdBus, eb, cb, qm, frameTracker);
}

void CopyDeviceToHostMemory::trigger(star::core::device::manager::ManagerCommandBuffer &bufferManager,
                                     star::core::CommandBus &cmdBus, const star::StarBuffers::Buffer &hostRayCutoff,
                                     const star::StarBuffers::Buffer &hostRayDist,
                                     const star::StarBuffers::Buffer &targetRayCutoffBuffer,
                                     const star::StarBuffers::Buffer &targetRayDistanceBuffer,
                                     star::core::device::manager::SemaphoreRecord *timeline,
                                     const uint64_t &valueToSignal)
{
    m_resources.signalValue = valueToSignal;
    m_resources.timelineRecord = timeline;
    m_resources.rayAtCutoff = &hostRayCutoff;
    m_resources.rayDistance = &hostRayDist;
    m_cpyCmds.trigger(bufferManager, cmdBus, targetRayCutoffBuffer, targetRayDistanceBuffer);
}
} // namespace image_metric_manager