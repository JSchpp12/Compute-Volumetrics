#pragma once

#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog::commands::color
{
/// Transitions the transmittance map from its compute-write layout to the
/// sampled layout used by the color pass. This is not a queue-family transfer.
class PreMemoryBarrierRecorder
{
  public:
    void recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                        vk::CommandBuffer cmdBuf) const noexcept;
};
} // namespace render_system::fog::commands::color
