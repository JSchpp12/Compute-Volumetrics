#include "render_system/fog/commands/color/PostMemoryBarrierRecorder.hpp"

namespace render_system::fog::commands::color
{
void PostMemoryBarrierRecorder::recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                                               vk::CommandBuffer cmdBuf) const noexcept
{
    BarrierBatch batch;
    if (std::holds_alternative<PostDifferentFamilies>(m_policy))
    {
        std::get<PostDifferentFamilies>(m_policy).build(vInfo, ft, batch);
    }

    const auto depInfo = batch.makeDependencyInfo();
    cmdBuf.pipelineBarrier2(depInfo);
}

} // namespace render_system::fog::commands::color
