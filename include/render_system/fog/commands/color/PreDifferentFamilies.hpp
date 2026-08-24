#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/struct/BarrierBatch.hpp"
#include "render_system/fog/struct/QueueFamilyIndices.hpp"

namespace render_system::fog::commands::color
{
/// Pre-barrier for the color pass. Acquires the offscreen render targets
/// (color/depth) and updated buffers (graphics -> compute). The shadow depth
/// acquire is NOT done here -- it is done by the transmittance pass (which
/// runs first and is the first shadow-depth consumer).
struct PreDifferentFamilies
{
    QueueFamilyIndices queueInfo;
    void build(const PassInfo &info, const star::common::FrameTracker &ft, BarrierBatch &batch) const noexcept;
};
} // namespace render_system::fog::commands::color
