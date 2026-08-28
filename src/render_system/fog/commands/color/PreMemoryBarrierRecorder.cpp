#include "render_system/fog/commands/color/PreMemoryBarrierRecorder.hpp"

namespace render_system::fog::commands::color
{
void PreMemoryBarrierRecorder::recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                                              vk::CommandBuffer cmdBuf) const noexcept
{
    BarrierBatch batch;
    if (std::holds_alternative<PreDifferentFamilies>(m_policy))
    {
        std::get<PreDifferentFamilies>(m_policy).build(vInfo, ft, batch);
    }

    // Transition the 3D transmittance map from eGeneral (written by the
    // transmittance precompute pass) to eShaderReadOnlyOptimal so the color
    // pass can read it as a sampler3D (combined-image-sampler). This is a
    // same-queue layout transition that must happen regardless of whether
    // graphics and compute share a queue family, so it is recorded here at
    // the recorder level rather than inside the queue-family-specific policy.
    // The precompute command buffer was submitted earlier in the same batch,
    // so the pipeline barrier's src scope covers its write.
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
