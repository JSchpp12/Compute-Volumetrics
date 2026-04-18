#pragma once

#include "renderer/VolumePassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace renderer
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

    void recordPreCommands(const VolumePassInfo &vInfo, vk::CommandBuffer cmdBuffer,
                           const star::common::FrameTracker &ft);
};
} // namespace renderer