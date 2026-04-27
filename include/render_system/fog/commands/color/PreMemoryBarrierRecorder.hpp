#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/color/PreDifferentFamilies.hpp"

#include <variant>

namespace render_system::fog::commands::color
{
using PreMemoryPolicy = std::variant<PreDifferentFamilies>;

class PreMemoryBarrierRecorder
{
    PreMemoryPolicy m_policy;

  public:
    explicit PreMemoryBarrierRecorder(PreMemoryPolicy policy) : m_policy(std::move(policy))
    {
    }

    void recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                        vk::CommandBuffer cmdBuf) const noexcept;
};
} // namespace render_system::fog::commands::color