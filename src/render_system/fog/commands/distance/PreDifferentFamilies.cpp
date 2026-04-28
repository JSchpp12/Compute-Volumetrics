#include "render_system/fog/commands/distance/PreDifferentFamilies.hpp"

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

static std::pair<std::array<vk::BufferMemoryBarrier2, 5>, uint32_t> GetBufferMemoryBarriers(
    const render_system::fog::PassInfo &vInfo, const star::common::FrameTracker &ft,
    const QueueFamilyIndices &qInfo) noexcept
{
    std::array<vk::BufferMemoryBarrier2, 5> barriers;
    uint32_t count{0};

    if (vInfo.transferWasRunLast)
    {
        barriers[0] = CreateMemoryBarrier(qInfo.transfer, qInfo.compute, vInfo.computeRayAtCutoffDistance);
        barriers[1] = CreateMemoryBarrier(qInfo.transfer, qInfo.compute, vInfo.computeRayDistance);
        count = 2;
    }

    return std::make_pair(barriers, count);
}

void PreDifferentFamilies::build(const PassInfo &info, const star::common::FrameTracker &ft,
                                 BarrierBatch &batch) const noexcept
{
    {
        const auto [barriers, count] =
            GetBufferMemoryBarriers(info, ft, queueInfo);

        for (uint8_t i{0}; i < count; i++)
        {
            batch.addBuffer(barriers[i]);
        }
    }
}
} // namespace render_system::fog::commands::distance