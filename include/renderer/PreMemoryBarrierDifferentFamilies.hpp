#pragma once

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace renderer
{
class PreMemoryBarrierDifferentFamilies
{
  public:
    void recordPreCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);
};
} // namespace renderer