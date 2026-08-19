#include "render_system/fog/policies/ShadowDepthAcquirePolicy.hpp"

namespace render_system::fog
{
void ShadowDepthOwnershipAcquire::build(vk::Image image, BarrierBatch &batch) const noexcept
{
    batch.addImage(vk::ImageMemoryBarrier2()
                       .setImage(image)
                       .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                       .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                       .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                       .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                       .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                       .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
                       .setSrcQueueFamilyIndex(graphicsQueueFamilyIndex)
                       .setDstQueueFamilyIndex(computeQueueFamilyIndex)
                       .setSubresourceRange(vk::ImageSubresourceRange()
                                                .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                .setBaseMipLevel(0)
                                                .setLevelCount(1)
                                                .setBaseArrayLayer(0)
                                                .setLayerCount(1)));
}

void ShadowDepthSameQueueTransition::build(vk::Image image, BarrierBatch &batch) const noexcept
{
    batch.addImage(vk::ImageMemoryBarrier2()
                       .setImage(image)
                       .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                       .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                       .setSrcStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                        vk::PipelineStageFlagBits2::eLateFragmentTests)
                       .setSrcAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
                       .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                       .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
                       .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setSubresourceRange(vk::ImageSubresourceRange()
                                                .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                .setBaseMipLevel(0)
                                                .setLevelCount(1)
                                                .setBaseArrayLayer(0)
                                                .setLayerCount(1)));
}

ShadowDepthAcquirePolicy makeShadowDepthAcquirePolicy(uint32_t graphics, uint32_t compute) noexcept
{
    return graphics != compute
               ? ShadowDepthAcquirePolicy{ShadowDepthOwnershipAcquire{graphics, compute}}
               : ShadowDepthAcquirePolicy{ShadowDepthSameQueueTransition{}};
}
} // namespace render_system::fog
