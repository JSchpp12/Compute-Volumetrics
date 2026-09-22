#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/color/PreDifferentFamilies.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog::commands::color
{
/// Acquires the frame-input images and buffers on the compute queue before the
/// first compute dispatch. This is the graphics-to-compute ownership transfer
/// that must happen before any pipeline binds those resources.
class FrameInputPreMemoryBarrierRecorder
{
    PreDifferentFamilies m_policy;

  public:
    explicit FrameInputPreMemoryBarrierRecorder(PreDifferentFamilies policy) : m_policy(std::move(policy))
    {
    }

    void recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                        vk::CommandBuffer cmdBuf) const noexcept;
};
} // namespace render_system::fog::commands::color
