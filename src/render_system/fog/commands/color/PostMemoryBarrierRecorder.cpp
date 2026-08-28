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

    // Transition the 3D transmittance map back
    if (vInfo.transmittanceMap != VK_NULL_HANDLE)
    {
        batch.addImage(vk::ImageMemoryBarrier2()
                           .setImage(vInfo.transmittanceMap)
                           .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                           .setNewLayout(vk::ImageLayout::eGeneral)
                           .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                           .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
                           .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                           .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
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
