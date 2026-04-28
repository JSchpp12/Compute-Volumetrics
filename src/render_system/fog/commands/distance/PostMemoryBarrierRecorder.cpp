#include "render_system/fog/commands/distance/PostMemoryBarrierRecorder.hpp"

#include "render_system/fog/struct/BarrierBatch.hpp"

namespace render_system::fog::commands::distance
{
void PostMemoryBarrierRecorder::recordCommands(const PassInfo &info, const star::common::FrameTracker &ft,
                                               vk::CommandBuffer cmdBuf) const noexcept
{
    BarrierBatch batch;
    if (std::holds_alternative<PostDifferentFamilies>(m_policy))
    {
        std::get<PostDifferentFamilies>(m_policy).build(info, ft, batch);
    }

    if (!batch.empty())
        cmdBuf.pipelineBarrier2(batch.makeDependencyInfo());
}
} // namespace render_system::fog::commands::distance