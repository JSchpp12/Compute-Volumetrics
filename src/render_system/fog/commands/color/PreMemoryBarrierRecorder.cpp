#include "render_system/fog/commands/color/PreMemoryBarrierRecorder.hpp"

#include "render_system/fog/BarrierFactory.hpp"
#include "render_system/fog/struct/BarrierBatch.hpp"

namespace render_system::fog::commands::color
{
void PreMemoryBarrierRecorder::recordCommands(PassInfo &vInfo, const star::common::FrameTracker &ft,
                                              vk::CommandBuffer cmdBuf) const noexcept
{
    (void)ft;

    BarrierBatch batch;

    if (vInfo.transmittanceMap != VK_NULL_HANDLE)
    {
        batch.addImage(MakeTransmittanceMapToShaderReadBarrier(vInfo.transmittanceMap));
    }

    if (!batch.empty())
        cmdBuf.pipelineBarrier2(batch.makeDependencyInfo());
}
} // namespace render_system::fog::commands::color
