#include "render_system/fog/policies/ShadowDepthReleaseBackPolicy.hpp"

namespace render_system::fog
{
void ShadowDepthOwnershipReleaseBack::build(vk::Image image, BarrierBatch &batch) const noexcept
{
    batch.addImage(vk::ImageMemoryBarrier2()
                       .setImage(image)
                       .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                       .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                       .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                       .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
                       .setDstStageMask(vk::PipelineStageFlagBits2::eNone) // release
                       .setDstAccessMask(vk::AccessFlagBits2::eNone)
                       .setSrcQueueFamilyIndex(computeQueueFamilyIndex)
                       .setDstQueueFamilyIndex(graphicsQueueFamilyIndex)
                       .setSubresourceRange(vk::ImageSubresourceRange()
                                                .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                .setBaseMipLevel(0)
                                                .setLevelCount(1)
                                                .setBaseArrayLayer(0)
                                                .setLayerCount(1)));
}

ShadowDepthReleaseBackPolicy makeShadowDepthReleaseBackPolicy(uint32_t graphics, uint32_t compute) noexcept
{
    return graphics != compute ? ShadowDepthReleaseBackPolicy{ShadowDepthOwnershipReleaseBack{graphics, compute}}
                               : ShadowDepthReleaseBackPolicy{ShadowDepthSameQueueReleaseBackNoOp{}};
}

void ShadowDepthSameQueueReleaseBackNoOp::build(vk::Image, BarrierBatch &) const noexcept
{
}
} // namespace render_system::fog
