#pragma once

#include <array>
#include <cassert>
#include <utility>
#include <variant>

#include <vulkan/vulkan.hpp>

struct BarrierBatch
{
    std::array<vk::ImageMemoryBarrier2, 8> imageBarriers{};
    std::array<vk::BufferMemoryBarrier2, 8> bufferBarriers{};

    uint32_t imageCount{0};
    uint32_t bufferCount{0};

    BarrierBatch &addImage(vk::ImageMemoryBarrier2 barrier)
    {
        assert(imageCount < imageBarriers.size());
        imageBarriers[imageCount++] = std::move(barrier);

        return *this;
    }

    BarrierBatch &addBuffer(vk::BufferMemoryBarrier2 barrier)
    {
        assert(bufferCount < bufferBarriers.size());
        bufferBarriers[bufferCount++] = std::move(barrier);

        return *this;
    }

    bool empty() const
    {
        return imageCount == 0 && bufferCount == 0;
    }

    vk::DependencyInfo makeDependencyInfo() const
    {
        return vk::DependencyInfo()
            .setPImageMemoryBarriers(imageBarriers.data())
            .setImageMemoryBarrierCount(imageCount)
            .setPBufferMemoryBarriers(bufferBarriers.data())
            .setBufferMemoryBarrierCount(bufferCount);
    }
};