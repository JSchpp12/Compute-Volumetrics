#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/policies/ShadowDepthAcquirePolicy.hpp"
#include "render_system/fog/struct/BarrierBatch.hpp"
#include "render_system/fog/struct/QueueFamilyIndices.hpp"

namespace render_system::fog::commands::color
{
/// Pre-barrier for the color pass. Acquires the offscreen render targets (color/depth), the volume
/// output, and updated buffers (graphics -> compute). Each acquire is recorded only when no earlier
/// chunk recorded it yet -- the PassInfo ImageReady latch, which this build flips. The shadow depth
/// is normally acquired by the transmittance pass (which runs first and is the first shadow-depth
/// consumer). When that pass is skipped (linear/exponential fog), this barrier performs the acquire
/// instead: the color pass still binds the shadow-map descriptor set (set 3), so the image must be
/// in the sampled layout.
struct PreDifferentFamilies
{
    QueueFamilyIndices queueInfo;
    render_system::fog::ShadowDepthAcquirePolicy shadowDepthAcquire;
    void build(PassInfo &info, const star::common::FrameTracker &ft, BarrierBatch &batch) const noexcept;
};
} // namespace render_system::fog::commands::color
