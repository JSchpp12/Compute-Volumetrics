#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/struct/BarrierBatch.hpp"
#include "render_system/fog/struct/QueueFamilyIndices.hpp"

namespace render_system::fog::commands::color
{
struct PreDifferentFamilies
{
    QueueFamilyIndices queueInfo;
    void build(const PassInfo &info, const star::common::FrameTracker &ft, BarrierBatch &batch) const noexcept;
};
} // namespace render_system::fog::commands::color