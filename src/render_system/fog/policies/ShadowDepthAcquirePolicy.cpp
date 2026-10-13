#include "render_system/fog/policies/ShadowDepthAcquirePolicy.hpp"

#include "render_system/fog/BarrierFactory.hpp"

namespace render_system::fog
{
void ShadowDepthOwnershipAcquire::build(vk::Image image, BarrierBatch &batch) const noexcept
{
    batch.addImage(MakeShadowDepthAcquireBarrier(image, graphicsQueueFamilyIndex, computeQueueFamilyIndex));
}

void ShadowDepthSameQueueTransition::build(vk::Image image, BarrierBatch &batch) const noexcept
{
    batch.addImage(MakeShadowDepthSameQueueAcquireBarrier(image));
}

ShadowDepthAcquirePolicy makeShadowDepthAcquirePolicy(uint32_t graphics, uint32_t compute) noexcept
{
    return graphics != compute ? ShadowDepthAcquirePolicy{ShadowDepthOwnershipAcquire{graphics, compute}}
                               : ShadowDepthAcquirePolicy{ShadowDepthSameQueueTransition{}};
}
} // namespace render_system::fog
