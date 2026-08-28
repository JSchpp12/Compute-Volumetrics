#include "render_system/fog/commands/distance/PostDifferentFamilies.hpp"

namespace render_system::fog::commands::distance
{
static vk::BufferMemoryBarrier2 CreateMemoryBarrier(const uint32_t &srcQueue, const uint32_t &dstQueue,
                                                    vk::Buffer buffer) noexcept
{
    return vk::BufferMemoryBarrier2()
        .setBuffer(buffer)
        .setSize(vk::WholeSize)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setSrcQueueFamilyIndex(srcQueue)
        .setDstQueueFamilyIndex(dstQueue);
}

static std::pair<std::array<vk::BufferMemoryBarrier2, 2>, uint32_t> GetBufferMemoryBarriers(
    const render_system::fog::PassInfo &vInfo, const star::common::FrameTracker &ft,
    const QueueFamilyIndices &fInfo) noexcept
{
    std::array<vk::BufferMemoryBarrier2, 2> barriers;
    uint32_t count{0};

    if (vInfo.transferWasRunLast)
    {
        barriers[0] = CreateMemoryBarrier(fInfo.transfer, fInfo.compute, vInfo.computeRayAtCutoffDistance);
        barriers[1] = CreateMemoryBarrier(fInfo.transfer, fInfo.compute, vInfo.computeRayDistance);
        count = 2;
    }

    return std::make_pair(barriers, count);
}

// Release the offscreen render-to-color/depth images, the volume output (computeWriteToImage), and the
// shadow depth back to the graphics queue. The depth/visibility pass is the last compute reader of all
// of these (its Init runs after the color pass and reads them), so it owns their release-back. This
// mirrors color::PostDifferentFamilies, which emits these release-backs only when the depth pass does
// NOT run (color is then the last compute reader).
static std::pair<std::array<vk::ImageMemoryBarrier2, 3>, uint32_t> GetImageMemoryBarriers(
    const render_system::fog::PassInfo &vInfo, uint32_t graphicsIndex, uint32_t computeIndex) noexcept
{
    std::array<vk::ImageMemoryBarrier2, 3> barriers{
        vk::ImageMemoryBarrier2()
            .setImage(vInfo.terrainPassInfo.renderToColor)
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone) // release
            .setDstAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(computeIndex)
            .setDstQueueFamilyIndex(graphicsIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)),
        vk::ImageMemoryBarrier2()
            .setImage(vInfo.terrainPassInfo.renderToDepth)
            .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone) // release
            .setDstAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(computeIndex)
            .setDstQueueFamilyIndex(graphicsIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)),
        vk::ImageMemoryBarrier2()
            .setImage(vInfo.computeWriteToImage)
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone) // release
            .setDstAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(computeIndex)
            .setDstQueueFamilyIndex(graphicsIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(vk::RemainingMipLevels)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(vk::RemainingArrayLayers))};

    return std::make_pair(barriers, 3);
}

void PostDifferentFamilies::build(const PassInfo &info, const star::common::FrameTracker &ft,
                                  BarrierBatch &batch) const noexcept
{
    {
        const auto [imageBarriers, imageCount] =
            GetImageMemoryBarriers(info, queueFamilyInfo.graphics, queueFamilyInfo.compute);
        for (uint8_t i{0}; i < imageCount; i++)
        {
            batch.addImage(imageBarriers[i]);
        }
    }

    if (info.terrainPassInfo.renderToShadowDepth != VK_NULL_HANDLE)
    {
        std::visit([&](const auto &p) { p.build(info.terrainPassInfo.renderToShadowDepth, batch); },
                   shadowDepthReleaseBack);
    }

    const auto [barriers, count] = GetBufferMemoryBarriers(info, ft, queueFamilyInfo);
    for (uint8_t i{ 0 }; i < count; i++)
    {
        batch.addBuffer(barriers[i]); 
    }

}
} // namespace render_system::fog::commands::distance
