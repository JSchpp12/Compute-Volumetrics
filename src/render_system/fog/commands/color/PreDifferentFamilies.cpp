#include "render_system/fog/commands/color/PreDifferentFamilies.hpp"

#include "render_system/fog/BarrierFactory.hpp"

#include <variant>

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

static std::pair<std::array<vk::BufferMemoryBarrier2, 2>, uint32_t> GetBufferMemoryBarriers(
    const render_system::fog::PassInfo &vInfo, const star::common::FrameTracker &ft, uint32_t graphicsIndex,
    uint32_t transferIndex, uint32_t computeIndex) noexcept
{
    std::array<vk::BufferMemoryBarrier2, 2> barriers;
    uint32_t count{0};

    if (vInfo.globalCameraBuffer.has_value())
    {
        barriers[count]
            .setBuffer(vInfo.globalCameraBuffer.value())
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
        MakeOffscreenColorAcquireBarrier(vInfo.terrainPassInfo.renderToColor, graphicsIndex, computeIndex),
        MakeOffscreenDepthAcquireBarrier(vInfo.terrainPassInfo.renderToDepth, graphicsIndex, computeIndex),
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
            : MakeVolumeOutputAcquireBarrier(vInfo.computeWriteToImage, graphicsIndex, computeIndex)};

    return std::make_pair(barriers, 3);
}

void PreDifferentFamilies::build(PassInfo &info, const star::common::FrameTracker &ft,
                                 BarrierBatch &batch) const noexcept
{
    {
        const auto [imageBarriers, barrierCountImage] =
            GetImageMemoryBarriers(info, ft, queueInfo.graphics, queueInfo.transfer, queueInfo.compute);

        if (!info.imageReady.offscreenColor)
        {
            batch.addImage(imageBarriers[0]);
            info.imageReady.offscreenColor = true;
        }
        if (!info.imageReady.offscreenDepth)
        {
            batch.addImage(imageBarriers[1]);
            info.imageReady.offscreenDepth = true;
        }
        if (!info.imageReady.volumeOutput)
        {
            batch.addImage(imageBarriers[2]);
            info.imageReady.volumeOutput = true;
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

    // The transmittance precompute chunk normally records the shadow-depth acquire first (it runs
    // before the color chunk in this shared command buffer). When it was skipped (linear/exponential
    // fog) the latch is still false and the color pass acquires instead
    if (info.terrainPassInfo.renderToShadowDepth != VK_NULL_HANDLE && !info.imageReady.shadowDepth)
    {
        info.imageReady.shadowDepth = true;
        std::visit([&](const auto &p) { p.build(info.terrainPassInfo.renderToShadowDepth, batch); },
                   shadowDepthAcquire);
    }
}
} // namespace render_system::fog::commands::color
