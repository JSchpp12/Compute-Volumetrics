#pragma once

#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog::commands
{

class PreMemoryBarrierDifferentFamilies
{
    uint32_t m_computeQueueFamilyIndex;
    uint32_t m_graphicsQueueFamilyIndex;
    uint32_t m_transferQueueFamilyIndex;

  public:
    PreMemoryBarrierDifferentFamilies(uint32_t computeQueueFamilyIndex, uint32_t graphicsQueueFamilyIndex,
                                      uint32_t transferQueueFamilyIndex)
        : m_computeQueueFamilyIndex(std::move(computeQueueFamilyIndex)),
          m_graphicsQueueFamilyIndex(std::move(graphicsQueueFamilyIndex)),
          m_transferQueueFamilyIndex(std::move(transferQueueFamilyIndex))
    {
    }

    void recordPreCommands(const PassInfo &vInfo, vk::CommandBuffer cmdBuffer,
                           const star::common::FrameTracker &ft);
};
} // namespace renderer