#pragma once

#include "render_system/fog/struct/BarrierBatch.hpp"

#include <cstdint>
#include <variant>

#include <vulkan/vulkan.hpp>

namespace render_system::fog
{
// Forward (shadow -> compute): acquire the shadow depth on the compute queue so
// the volume can sample it. Emitted in the volume Pre (color::PreDifferentFamilies).
struct ShadowDepthOwnershipAcquire
{
    uint32_t graphicsQueueFamilyIndex;
    uint32_t computeQueueFamilyIndex;

    void build(vk::Image image, BarrierBatch &batch) const noexcept;
};

struct ShadowDepthSameQueueTransition
{
    void build(vk::Image image, BarrierBatch &batch) const noexcept;
};

using ShadowDepthAcquirePolicy = std::variant<ShadowDepthOwnershipAcquire, ShadowDepthSameQueueTransition>;

ShadowDepthAcquirePolicy makeShadowDepthAcquirePolicy(uint32_t graphics, uint32_t compute) noexcept;
} // namespace render_system::fog
