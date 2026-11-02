#include "render_system/fog/commands/color/PostDifferentFamilies.hpp"

#include "render_system/fog/BarrierFactory.hpp"

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
        MakeOffscreenColorReleaseBarrier(vInfo.terrainPassInfo.renderToColor, computeIndex, graphicsIndex),
        MakeOffscreenDepthReleaseBarrier(vInfo.terrainPassInfo.renderToDepth, computeIndex, graphicsIndex),
        MakeVolumeOutputReleaseBarrier(vInfo.computeWriteToImage, computeIndex, graphicsIndex)};

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

    // imageBarriers[0],[1] = offscreen render-to-color/depth. When the depth/visibility pass runs it reads
    // these after this (color) pass, so color only releases them back to graphics when it is the last reader
    // (depth pass disabled -- non-marched fog). When the depth pass runs it owns the release-back
    // (distance::PostDifferentFamilies). imageBarriers[2] = computeWriteToImage and the shadow depth
    // are also deferred to the distance post when the depth pass runs -- the distance Init reads both
    // after the color pass, so releasing them here would hand them back to graphics before the distance
    // pass reads them. Only release them back from the color post when the depth pass does NOT run
    // (color is then the last compute reader).
    if (!info.depthPassWillRun)
    {
        batch.addImage(imageBarriers[0]);
        batch.addImage(imageBarriers[1]);
        batch.addImage(imageBarriers[2]);
    }

    if (info.terrainPassInfo.renderToShadowDepth != VK_NULL_HANDLE && !info.depthPassWillRun)
    {
        std::visit([&](const auto &p) { p.build(info.terrainPassInfo.renderToShadowDepth, batch); },
                   shadowDepthReleaseBack);
    }

    const auto [memoryBarriers, barrierCountBuffer] =
        GetBufferMemoryBarriers(info, queueInfo.transfer, queueInfo.compute);
    for (uint8_t i{0}; i < barrierCountBuffer; i++)
    {
        batch.addBuffer(memoryBarriers[i]);
    }
}

} // namespace render_system::fog::commands::color