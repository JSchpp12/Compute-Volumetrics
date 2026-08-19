#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/struct/BarrierBatch.hpp"
#include "render_system/fog/struct/QueueFamilyIndices.hpp"
#include "render_system/fog/policies/ShadowDepthReleaseBackPolicy.hpp"

namespace render_system::fog::commands::color
{
struct PostDifferentFamilies
{
    QueueFamilyIndices queueInfo;
    render_system::fog::ShadowDepthReleaseBackPolicy shadowDepthReleaseBack;

    void build(const PassInfo &info, const star::common::FrameTracker &ft, BarrierBatch &batch) const noexcept;
};
} // namespace render_system::fog::commands::color
