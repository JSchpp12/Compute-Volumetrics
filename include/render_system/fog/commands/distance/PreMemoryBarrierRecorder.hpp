#pragma once

#include "render_system/fog/commands/distance/PreDifferentFamilies.hpp"
#include "render_system/fog/struct/PassInfo.hpp"

namespace render_system::fog::commands::distance
{
using PreMemoryPolicy = std::variant<PreDifferentFamilies>;

class PreMemoryBarrierRecorder
{
    PreMemoryPolicy m_policy;

  public:
    explicit PreMemoryBarrierRecorder(PreMemoryPolicy policy) : m_policy(std::move(policy))
    {
    }

    void recordCommands(PassInfo &info, const star::common::FrameTracker &ft, vk::CommandBuffer cmdBuf) const noexcept;
};
} // namespace render_system::fog::commands::distance