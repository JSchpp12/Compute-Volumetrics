#pragma once

#include <vulkan/vulkan.hpp>

#include <array>

namespace render_system::fog
{
// all chunks will share this value
struct WaitInfo
{
    std::array<vk::SemaphoreSubmitInfo, 5> info;
    uint32_t count{0};
};

// every chunk will have its own
struct SignalInfo
{
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    uint64_t signalValue{0};
};
} // namespace render_system::fog