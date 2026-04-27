#include "render_system/fog/commands/color/PreDifferentFamilies.hpp"

namespace render_system::fog::commands::color
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

static std::pair<std::array<vk::BufferMemoryBarrier2, 5>, uint32_t> GetBufferMemoryBarriers(
    const render_system::fog::PassInfo &vInfo, const star::common::FrameTracker &ft, uint32_t graphicsIndex,
    uint32_t transferIndex, uint32_t computeIndex) noexcept
{
    std::array<vk::BufferMemoryBarrier2, 5> barriers;
    uint32_t count{0};

    if (vInfo.transferWasRunLast)
    {
        barriers[0] = CreateMemoryBarrier(transferIndex, computeIndex, vInfo.computeRayAtCutoffDistance);
        barriers[1] = CreateMemoryBarrier(transferIndex, computeIndex, vInfo.computeRayDistance);
        count = 2;
    }

    if (vInfo.globalCameraBuffer.has_value())
    {
        barriers[count]
            .setBuffer(std::move(vInfo.globalCameraBuffer.value()))
            .setSize(vk::WholeSize)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);

        count++;
    }

    if (vInfo.fogControllerBuffer.has_value())
    {
        barriers[count]
            .setBuffer(vInfo.fogControllerBuffer.value())
            .setSize(vk::WholeSize)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
        count++;
    }

    return std::make_pair(barriers, count);
}

static std::pair<std::array<vk::ImageMemoryBarrier2, 3>, uint32_t> GetImageMemoryBarriers(
    const render_system::fog::PassInfo &vInfo, const star::common::FrameTracker &ft, uint32_t graphicsIndex,
    uint32_t transferIndex, uint32_t computeIndex)
{
    std::array<vk::ImageMemoryBarrier2, 3> barriers{
        vk::ImageMemoryBarrier2()
            .setImage(vInfo.terrainPassInfo.renderToColor)
            .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(graphicsIndex)
            .setDstQueueFamilyIndex(computeIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)),
        vk::ImageMemoryBarrier2()
            .setImage(vInfo.terrainPassInfo.renderToDepth)
            .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(graphicsIndex)
            .setDstQueueFamilyIndex(computeIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)
                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth)),
        ft.getCurrent().getNumTimesFrameProcessed() == 0
            ? vk::ImageMemoryBarrier2()
                  .setImage(vInfo.computeWriteToImage)
                  .setOldLayout(vk::ImageLayout::eUndefined)
                  .setNewLayout(vk::ImageLayout::eGeneral)
                  .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                  .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                  .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                  .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                  .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                  .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
                  .setSubresourceRange(vk::ImageSubresourceRange()
                                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                           .setBaseMipLevel(0)
                                           .setLevelCount(1)
                                           .setBaseArrayLayer(0)
                                           .setLayerCount(1))
            : vk::ImageMemoryBarrier2()
                  .setImage(vInfo.computeWriteToImage)
                  .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                  .setNewLayout(vk::ImageLayout::eGeneral)
                  .setSrcQueueFamilyIndex(graphicsIndex)
                  .setDstQueueFamilyIndex(computeIndex)
                  .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                  .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                  .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                  .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
                  .setSubresourceRange(vk::ImageSubresourceRange()
                                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                           .setBaseMipLevel(0)
                                           .setLevelCount(1)
                                           .setBaseArrayLayer(0)
                                           .setLayerCount(1))};

    return std::make_pair(barriers, 3);
}

void PreDifferentFamilies::build(const PassInfo &info, const star::common::FrameTracker &ft,
                                 BarrierBatch &batch) const noexcept
{
    {
        const auto [imageBarriers, barrierCountImage] =
            GetImageMemoryBarriers(info, ft, queueInfo.graphics, queueInfo.transfer, queueInfo.compute);

        for (uint8_t i{0}; i < barrierCountImage; i++)
        {
            batch.addImage(imageBarriers[i]);
        }
    }

    {
        const auto [bufferBarriers, barrierCountBuffer] =
            GetBufferMemoryBarriers(info, ft, queueInfo.graphics, queueInfo.transfer, queueInfo.compute);

        for (uint8_t i{0}; i < barrierCountBuffer; i++)
        {
            batch.addBuffer(bufferBarriers[i]);
        }
    }
}
} // namespace render_system::fog::commands::color