#include "render_system/fog/commands/transmittance/PreMemoryBarrierRecorder.hpp"

#include "render_system/fog/BarrierFactory.hpp"

namespace render_system::fog::commands::transmittance
{
void ShadowDepthAcquire::build(PassInfo &info, BarrierBatch &batch) const noexcept
{
    if (info.terrainPassInfo.renderToShadowDepth != VK_NULL_HANDLE && !info.imageReady.shadowDepth)
    {
        info.imageReady.shadowDepth = true;
        std::visit([&](const auto &p) { p.build(info.terrainPassInfo.renderToShadowDepth, batch); }, policy);
    }
}

void PreMemoryBarrierRecorder::recordPrepMemoryBarriers(PassInfo &vInfo, const star::common::FrameTracker &ft,
                                                        vk::CommandBuffer cmdBuf) const noexcept
{
    BarrierBatch batch;
    if (std::holds_alternative<ShadowDepthAcquire>(m_policy))
    {
        std::get<ShadowDepthAcquire>(m_policy).build(vInfo, batch);
    }

    assert(vInfo.transmittanceMap != VK_NULL_HANDLE && "The tranmittance map texture needs to be provided");
    // transition image layout to transfer dst
    batch.addImage(MakeTransmittanceMapToTransferDstBarrier(vInfo.transmittanceMap));

    if (!batch.empty())
        cmdBuf.pipelineBarrier2(batch.makeDependencyInfo());
}

void PreMemoryBarrierRecorder::recordTransmittanceMapReset(const PassInfo &vInfo,
                                                           vk::CommandBuffer cmdBuf) const noexcept
{
    // erase the image with 0s
    const auto resetRange = vk::ImageSubresourceRange()
                                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setBaseMipLevel(0)
                                .setLevelCount(vk::RemainingMipLevels)
                                .setBaseArrayLayer(0)
                                .setLayerCount(vk::RemainingArrayLayers);
    cmdBuf.clearColorImage(vInfo.transmittanceMap, vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue{},
                           resetRange);

    // transition image layout back to general
    {
        const auto backBarrier = MakeTransmittanceMapToGeneralBarrier(vInfo.transmittanceMap);
        cmdBuf.pipelineBarrier2(
            vk::DependencyInfo().setImageMemoryBarrierCount(1).setPImageMemoryBarriers(&backBarrier));
    }
}

void PreMemoryBarrierRecorder::recordCommands(PassInfo &vInfo, const star::common::FrameTracker &ft,
                                              vk::CommandBuffer cmdBuf) const noexcept

{
    // record all of the "standard" barriers
    recordPrepMemoryBarriers(vInfo, ft, cmdBuf);
    recordTransmittanceMapReset(vInfo, cmdBuf);
}
} // namespace render_system::fog::commands::transmittance