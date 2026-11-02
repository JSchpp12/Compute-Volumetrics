#include "render_system/fog/policies/ShadowDepthReleaseBackPolicy.hpp"

#include "render_system/fog/BarrierFactory.hpp"

namespace render_system::fog
{
void ShadowDepthOwnershipReleaseBack::build(vk::Image image, BarrierBatch &batch) const noexcept
{
    batch.addImage(MakeShadowDepthReleaseBarrier(image, computeQueueFamilyIndex, graphicsQueueFamilyIndex));
}

ShadowDepthReleaseBackPolicy makeShadowDepthReleaseBackPolicy(uint32_t graphics, uint32_t compute) noexcept
{
    return graphics != compute ? ShadowDepthReleaseBackPolicy{ShadowDepthOwnershipReleaseBack{graphics, compute}}
                               : ShadowDepthReleaseBackPolicy{ShadowDepthSameQueueReleaseBackNoOp{}};
}

void ShadowDepthSameQueueReleaseBackNoOp::build(vk::Image, BarrierBatch &) const noexcept
{
}
} // namespace render_system::fog
