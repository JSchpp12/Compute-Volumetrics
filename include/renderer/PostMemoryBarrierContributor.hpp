#pragma once

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace renderer
{
class PostMemoryBarrierContributor
{
  public:
    void recordPostCommands(vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);
};
} // namespace renderer