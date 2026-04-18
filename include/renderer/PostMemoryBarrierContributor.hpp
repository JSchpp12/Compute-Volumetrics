#pragma once

#include "renderer/PostMemoryBarrierDifferentFamilies.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <variant>

namespace renderer
{
class PostMemoryBarrierContributor
{
    std::variant<PostMemoryBarrierDifferentFamilies> m_approach;

  public:
    explicit PostMemoryBarrierContributor(PostMemoryBarrierDifferentFamilies approach) : m_approach(std::move(approach))
    {
    }
    
    void recordPostCommands(vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);
};
} // namespace renderer