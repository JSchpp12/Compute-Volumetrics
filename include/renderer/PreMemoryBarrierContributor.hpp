#pragma once

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace renderer
{
class PreMemoryBarrierContributor
{
  public:
    void recordPreCommands(vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);
};
} // namespace renderer