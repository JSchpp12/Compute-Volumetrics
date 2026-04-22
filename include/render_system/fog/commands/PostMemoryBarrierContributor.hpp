#pragma once

#include "render_system/fog/commands/PostMemoryBarrierDifferentFamilies.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <variant>

namespace render_system::fog::commands
{
class PostMemoryBarrierContributor
{
    std::variant<PostMemoryBarrierDifferentFamilies> m_approach;

  public:
    explicit PostMemoryBarrierContributor(PostMemoryBarrierDifferentFamilies approach) : m_approach(std::move(approach))
    {
    }

    void recordPostCommands(const PassInfo &vInfo, vk::CommandBuffer cmdBuf,
                            const star::common::FrameTracker &ft);
};
} // namespace renderer