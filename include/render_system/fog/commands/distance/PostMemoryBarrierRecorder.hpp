#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/distance/PostDifferentFamilies.hpp"

#include <variant>

namespace render_system::fog::commands::distance
{
class PostMemoryBarrierRecorder
{
    std::variant<PostDifferentFamilies> m_policy;

  public:
    explicit PostMemoryBarrierRecorder(std::variant<PostDifferentFamilies> policy) : m_policy(std::move(policy))
    {
    }

    void recordCommands(const PassInfo &info, const star::common::FrameTracker &ft,
                        vk::CommandBuffer cmdBuf) const noexcept;
};
} // namespace render_system::fog::commands::distance