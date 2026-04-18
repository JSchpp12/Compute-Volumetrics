#pragma once

#include "renderer/VolumePassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace renderer
{
class PostMemoryBarrierDifferentFamilies
{
  public:
    void recordPostCommands(const VolumePassInfo &tInfo, vk::CommandBuffer commandBuffer,
                            const star::common::FrameTracker &ft);
};
} // namespace renderer