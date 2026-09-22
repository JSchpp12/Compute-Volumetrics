#include "render_system/fog/commands/color/FrameInputPreMemoryBarrierRecorder.hpp"

#include "render_system/fog/struct/BarrierBatch.hpp"

namespace render_system::fog::commands::color
{
void FrameInputPreMemoryBarrierRecorder::recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                                                        vk::CommandBuffer cmdBuf) const noexcept
{
    BarrierBatch batch;
    m_policy.build(vInfo, ft, batch);

    if (!batch.empty())
        cmdBuf.pipelineBarrier2(batch.makeDependencyInfo());
}
} // namespace render_system::fog::commands::color
