#include "service/detail/image_metric_manager/CopyDeviceToHostMemory.hpp"

namespace service::image_metric_manager
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
                                     star::core::CommandBus &cmdBus)
{
    m_cpyCmds.trigger(bufferManager, cmdBus);
}
} // namespace service::image_metric_manager