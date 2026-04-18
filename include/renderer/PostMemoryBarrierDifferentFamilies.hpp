#pragma once

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace renderer
{
class PostMemoryBarrierDifferentFamilies
{
    public:
    void recordPostCommands(vk::CommandBuffer commandBuffer, const star::common::FrameTracker &ft); 

};
} // namespace renderer