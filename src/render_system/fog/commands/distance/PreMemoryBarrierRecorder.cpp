#include "render_system/fog/commands/distance/PreMemoryBarrierRecorder.hpp"

namespace render_system::fog::commands::distance
{
void PreMemoryBarrierRecorder::recordCommands(const PassInfo &info, const star::common::FrameTracker &ft,
                                              vk::CommandBuffer cmdBuf) const noexcept
{
    BarrierBatch batch;
    if (std::holds_alternative<PreDifferentFamilies>(m_policy))
    {
        std::get<PreDifferentFamilies>(m_policy).build(info, ft, batch);
    }

    if (!batch.empty())
        cmdBuf.pipelineBarrier2(batch.makeDependencyInfo());
}
} // namespace render_system::fog::commands::distance