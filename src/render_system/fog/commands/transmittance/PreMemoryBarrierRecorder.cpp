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

void PreMemoryBarrierRecorder::recordPrepMemoryBarriers(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                                                        vk::CommandBuffer cmdBuf) const noexcept
{
    BarrierBatch batch;
    if (std::holds_alternative<ShadowDepthAcquire>(m_policy))
    {
        std::get<ShadowDepthAcquire>(m_policy).build(vInfo, batch);
    }

    assert(vInfo.transmittanceMap != VK_NULL_HANDLE && "The tranmittance map texture needs to be provided");
    // transition image layout to transfer dst
    batch.addImage(vk::ImageMemoryBarrier2()
                       .setImage(vInfo.transmittanceMap)
                       .setOldLayout(vk::ImageLayout::eGeneral)
                       .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                       .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
                       .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
                       .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
                       .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                       .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                       .setSubresourceRange(vk::ImageSubresourceRange()
                                                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                .setBaseMipLevel(0)
                                                .setLevelCount(vk::RemainingMipLevels)
                                                .setBaseArrayLayer(0)
                                                .setLayerCount(vk::RemainingArrayLayers))

    );

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
        const auto backBarrier = vk::ImageMemoryBarrier2()
                                     .setImage(vInfo.transmittanceMap)
                                     .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                                     .setNewLayout(vk::ImageLayout::eGeneral)
                                     .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                                     .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                                     .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                                     .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
                                     .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                     .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                     .setSubresourceRange(vk::ImageSubresourceRange()
                                                              .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                              .setBaseMipLevel(0)
                                                              .setLevelCount(vk::RemainingMipLevels)
                                                              .setBaseArrayLayer(0)
                                                              .setLayerCount(vk::RemainingArrayLayers));
        cmdBuf.pipelineBarrier2(
            vk::DependencyInfo().setImageMemoryBarrierCount(1).setPImageMemoryBarriers(&backBarrier));
    }
}

void PreMemoryBarrierRecorder::recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                                              vk::CommandBuffer cmdBuf) const noexcept

{
    // record all of the "standard" barriers
    recordPrepMemoryBarriers(vInfo, ft, cmdBuf);
    recordTransmittanceMapReset(vInfo, cmdBuf);
}
} // namespace render_system::fog::commands::transmittance