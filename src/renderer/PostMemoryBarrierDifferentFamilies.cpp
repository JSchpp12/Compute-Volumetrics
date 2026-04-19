#include "renderer/PostMemoryBarrierDifferentFamilies.hpp"

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
    const renderer::VolumePassInfo &vInfo, uint32_t graphicsIndex, uint32_t computeIndex)
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
    const renderer::VolumePassInfo &vInfo, uint32_t transferIndex, uint32_t computeIndex)
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

void renderer::PostMemoryBarrierDifferentFamilies::recordPostCommands(const VolumePassInfo &vInfo,
                                                                      vk::CommandBuffer cmdBuf,
                                                                      const star::common::FrameTracker &ft)
{
    const auto [imageBarriers, barrierCountImage] =
        GetImageMemoryBarriers(vInfo, m_graphicsQueueFamilyIndex, m_computeQueueFamilyIndex);

    const auto [memoryBarriers, barrierCountBuffer] =
        GetBufferMemoryBarriers(vInfo, m_transferQueueFamilyIndex, m_computeQueueFamilyIndex);

    const auto depInfo = vk::DependencyInfo()
                             .setPImageMemoryBarriers(imageBarriers.data())
                             .setImageMemoryBarrierCount(barrierCountImage)
                             .setPBufferMemoryBarriers(memoryBarriers.data())
                             .setBufferMemoryBarrierCount(barrierCountBuffer);

    cmdBuf.pipelineBarrier2(depInfo);
}