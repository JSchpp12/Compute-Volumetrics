#pragma once

#include "render_system/fog/struct/BarrierBatch.hpp"

#include <cstdint>
#include <variant>

#include <vulkan/vulkan.hpp>

namespace render_system::fog
{
// Reverse (compute -> shadow): release the shadow depth back to graphics after the
// volume sampled it. Emitted in the volume Post (color::PostDifferentFamilies).
// For the same-queue case this is a no-op; the shadow Pre does the single transition back.
struct ShadowDepthOwnershipReleaseBack
{
    uint32_t graphicsQueueFamilyIndex;
    uint32_t computeQueueFamilyIndex;

    void build(vk::Image image, BarrierBatch &batch) const noexcept;
};

struct ShadowDepthSameQueueReleaseBackNoOp
{
    void build(vk::Image, BarrierBatch &) const noexcept;
};

using ShadowDepthReleaseBackPolicy = std::variant<ShadowDepthOwnershipReleaseBack, ShadowDepthSameQueueReleaseBackNoOp>;

ShadowDepthReleaseBackPolicy makeShadowDepthReleaseBackPolicy(uint32_t graphics, uint32_t compute) noexcept;
} // namespace render_system::fog
