#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/color/PostDifferentFamilies.hpp"

#include <variant>

namespace render_system::fog::commands::color
{
using PostMemoryPolicy = std::variant<PostDifferentFamilies>;

class PostMemoryBarrierRecorder
{
    PostMemoryPolicy m_policy;

  public:
    PostMemoryBarrierRecorder(PostMemoryPolicy policy) : m_policy(std::move(policy))
    {
    }

    void recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                        vk::CommandBuffer cmdBuf) const noexcept;
};
} // namespace render_system::fog::commands::color