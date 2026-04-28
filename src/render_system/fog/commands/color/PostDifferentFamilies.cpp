#include "render_system/fog/commands/color/PostDifferentFamilies.hpp"

namespace render_system::fog::commands::color
{

inline static vk::BufferMemoryBarrier2 CreateMemoryBarrier(uint32_t srcQueue, uint32_t dstQueue, vk::Buffer buffer)
{
    return vk::BufferMemoryBarrier2()
        .setBuffer(buffer)
        .setSize(vk::WholeSize)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSrcQueueFamilyIndex(std::move(srcQueue))
        .setDstQueueFamilyIndex(std::move(dstQueue));
}

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
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
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
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
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
            .setSrcQueueFamilyIndex(computeIndex)
            .setDstQueueFamilyIndex(graphicsIndex)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
            .setDstAccessMask(vk::AccessFlagBits2::eNone)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(vk::RemainingMipLevels)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(vk::RemainingArrayLayers))

    };

    return std::make_pair(barriers, 3);
}

static std::pair<std::array<vk::BufferMemoryBarrier2, 5>, uint32_t> GetBufferMemoryBarriers(
    const render_system::fog::PassInfo &vInfo, uint32_t transferIndex, uint32_t computeIndex) noexcept
{
    std::array<vk::BufferMemoryBarrier2, 5> barriers;
    uint32_t count{0};

    if (vInfo.transferWillBeRunThisFrame)
    {
        barriers[0] = CreateMemoryBarrier(computeIndex, transferIndex, vInfo.computeRayDistance);
        barriers[1] = CreateMemoryBarrier(computeIndex, transferIndex, vInfo.computeRayAtCutoffDistance);
        count = 2;
    }

    return std::make_pair(barriers, count);
}

void PostDifferentFamilies::build(const PassInfo &info, const star::common::FrameTracker &ft,
                                  BarrierBatch &batch) const noexcept
{
    const auto [imageBarriers, barrierCountImage] = GetImageMemoryBarriers(info, queueInfo.graphics, queueInfo.compute);

    for (uint8_t i{0}; i < barrierCountImage; i++)
    {
        batch.addImage(imageBarriers[i]);
    }

    const auto [memoryBarriers, barrierCountBuffer] =
        GetBufferMemoryBarriers(info, queueInfo.transfer, queueInfo.compute);
    for (uint8_t i{0}; i < barrierCountBuffer; i++)
    {
        batch.addBuffer(memoryBarriers[i]);
    }
}

} // namespace render_system::fog::commands::color