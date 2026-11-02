#pragma once

#include "render_system/fog/policies/ShadowDepthAcquirePolicy.hpp"
#include "render_system/fog/struct/BarrierBatch.hpp"
#include "render_system/fog/struct/PassInfo.hpp"
#include "render_system/fog/struct/QueueFamilyIndices.hpp"

namespace render_system::fog::commands::color
{
/// @brief Pre-barrier for the color pass. Acquires the offscreen render targets (color/depth), the volume output, and
/// updated buffers (graphics -> compute).
struct PreDifferentFamilies
{
    QueueFamilyIndices queueInfo;
    render_system::fog::ShadowDepthAcquirePolicy shadowDepthAcquire;
    void build(PassInfo &info, const star::common::FrameTracker &ft, BarrierBatch &batch) const noexcept;
};
} // namespace render_system::fog::commands::color
