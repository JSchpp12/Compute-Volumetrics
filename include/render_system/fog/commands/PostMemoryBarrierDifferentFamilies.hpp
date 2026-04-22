#pragma once

#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog::commands
{
class PostMemoryBarrierDifferentFamilies
{
    uint32_t m_computeQueueFamilyIndex{0};
    uint32_t m_graphicsQueueFamilyIndex{0};
    uint32_t m_transferQueueFamilyIndex{0};

  public:
    PostMemoryBarrierDifferentFamilies(uint32_t computeIndex, uint32_t graphicsIndex, uint32_t transferIndex)
        : m_computeQueueFamilyIndex(std::move(computeIndex)), m_graphicsQueueFamilyIndex(std::move(graphicsIndex)),
          m_transferQueueFamilyIndex(std::move(transferIndex))

    {
    }

    void recordPostCommands(const PassInfo &tInfo, vk::CommandBuffer commandBuffer,
                            const star::common::FrameTracker &ft);
};
} // namespace render_system::fog::commands