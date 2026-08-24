#include "render_system/fog/commands/transmittance/PreMemoryBarrierRecorder.hpp"

namespace render_system::fog::commands::transmittance
{
void ShadowDepthAcquire::build(const PassInfo &info, BarrierBatch &batch) const noexcept
{
    if (info.terrainPassInfo.renderToShadowDepth != VK_NULL_HANDLE)
    {
        std::visit([&](const auto &p) { p.build(info.terrainPassInfo.renderToShadowDepth, batch); }, policy);
    }
}

void PreMemoryBarrierRecorder::recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                                               vk::CommandBuffer cmdBuf) const noexcept
{
    BarrierBatch batch;
    if (std::holds_alternative<ShadowDepthAcquire>(m_policy))
    {
        std::get<ShadowDepthAcquire>(m_policy).build(vInfo, batch);
    }

    if (!batch.empty())
        cmdBuf.pipelineBarrier2(batch.makeDependencyInfo());
}
} // namespace render_system::fog::commands::transmittance