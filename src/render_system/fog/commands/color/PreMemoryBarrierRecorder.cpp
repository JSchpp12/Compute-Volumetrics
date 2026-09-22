#include "render_system/fog/commands/color/PreMemoryBarrierRecorder.hpp"

#include "render_system/fog/struct/BarrierBatch.hpp"

namespace render_system::fog::commands::color
{
void PreMemoryBarrierRecorder::recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                                              vk::CommandBuffer cmdBuf) const noexcept
{
    (void)ft;

    BarrierBatch batch;

    if (vInfo.transmittanceMap != VK_NULL_HANDLE)
    {
        batch.addImage(vk::ImageMemoryBarrier2()
                           .setImage(vInfo.transmittanceMap)
                           .setOldLayout(vk::ImageLayout::eGeneral)
                           .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                           .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                           .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
                           .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                           .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
                           .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                           .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                           .setSubresourceRange(vk::ImageSubresourceRange()
                                                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                    .setBaseMipLevel(0)
                                                    .setLevelCount(vk::RemainingMipLevels)
                                                    .setBaseArrayLayer(0)
                                                    .setLayerCount(vk::RemainingArrayLayers)));
    }

    if (!batch.empty())
        cmdBuf.pipelineBarrier2(batch.makeDependencyInfo());
}
} // namespace render_system::fog::commands::color
