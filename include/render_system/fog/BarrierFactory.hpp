#pragma once

#include <cstdint>

#include <vulkan/vulkan.hpp>

namespace render_system::fog
{
// ---------------------------------------------------------------------------------------------
// Acquires: graphics -> compute
// ---------------------------------------------------------------------------------------------

/// @brief Builds an ownership acquire for the offscreen color attachment (color attachment optimal -> general),
/// released by graphics and acquired by compute for shader reads.
/// @param image The offscreen color image to acquire.
/// @param graphicsQueue Queue family index releasing ownership (graphics).
/// @param computeQueue Queue family index acquiring ownership (compute).
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeOffscreenColorAcquireBarrier(vk::Image image, uint32_t graphicsQueue,
                                                                uint32_t computeQueue) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSrcQueueFamilyIndex(graphicsQueue)
        .setDstQueueFamilyIndex(computeQueue)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(1)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(1));
}

/// @brief Builds an ownership acquire for the offscreen depth attachment (depth attachment optimal -> shader read
/// only), released by graphics and acquired by compute.
/// @param image The offscreen depth image to acquire.
/// @param graphicsQueue Queue family index releasing ownership (graphics).
/// @param computeQueue Queue family index acquiring ownership (compute).
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeOffscreenDepthAcquireBarrier(vk::Image image, uint32_t graphicsQueue,
                                                                uint32_t computeQueue) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSrcQueueFamilyIndex(graphicsQueue)
        .setDstQueueFamilyIndex(computeQueue)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(1)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(1));
}

/// @brief Builds an ownership acquire for the volume output image (shader read only -> general), released by graphics
/// and acquired by compute for shader writes.
/// @param image The volume output image to acquire.
/// @param graphicsQueue Queue family index releasing ownership (graphics).
/// @param computeQueue Queue family index acquiring ownership (compute).
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeVolumeOutputAcquireBarrier(vk::Image image, uint32_t graphicsQueue,
                                                              uint32_t computeQueue) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setSrcQueueFamilyIndex(graphicsQueue)
        .setDstQueueFamilyIndex(computeQueue)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(1)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(1));
}

/// @brief Builds an ownership acquire for the terrain shadow depth image (depth attachment optimal -> shader read
/// only), released by graphics and acquired by compute. Identical in shape to the offscreen depth acquire; kept
/// separate so call sites remain explicit about the resource they acquire.
/// @param image The terrain shadow depth image to acquire.
/// @param graphicsQueue Queue family index releasing ownership (graphics).
/// @param computeQueue Queue family index acquiring ownership (compute).
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeShadowDepthAcquireBarrier(vk::Image image, uint32_t graphicsQueue,
                                                             uint32_t computeQueue) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSrcQueueFamilyIndex(graphicsQueue)
        .setDstQueueFamilyIndex(computeQueue)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(1)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(1));
}

// ---------------------------------------------------------------------------------------------
// Releases: compute -> graphics
// ---------------------------------------------------------------------------------------------

/// @brief Builds an ownership release for the offscreen color attachment (general -> color attachment optimal),
/// released by compute and acquired back by graphics.
/// @param image The offscreen color image to release.
/// @param computeQueue Queue family index releasing ownership (compute).
/// @param graphicsQueue Queue family index acquiring ownership back (graphics).
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeOffscreenColorReleaseBarrier(vk::Image image, uint32_t computeQueue,
                                                                uint32_t graphicsQueue) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eGeneral)
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSrcQueueFamilyIndex(computeQueue)
        .setDstQueueFamilyIndex(graphicsQueue)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(1)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(1));
}

/// @brief Builds an ownership release for the offscreen depth image (shader read only -> depth attachment optimal),
/// released by compute and acquired back by graphics.
/// @param image The offscreen depth image to release.
/// @param computeQueue Queue family index releasing ownership (compute).
/// @param graphicsQueue Queue family index acquiring ownership back (graphics).
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeOffscreenDepthReleaseBarrier(vk::Image image, uint32_t computeQueue,
                                                                uint32_t graphicsQueue) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSrcQueueFamilyIndex(computeQueue)
        .setDstQueueFamilyIndex(graphicsQueue)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(1)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(1));
}

/// @brief Builds an ownership release for the volume output image (general -> shader read only), released by compute
/// and acquired back by graphics.
/// @param image The volume output image to release.
/// @param computeQueue Queue family index releasing ownership (compute).
/// @param graphicsQueue Queue family index acquiring ownership back (graphics).
/// @param computeAccess Which compute accesses are released: the color pass only wrote the image (eShaderWrite), while
/// the depth/visibility pass also read it during Init (eShaderRead | eShaderWrite).
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeVolumeOutputReleaseBarrier(
    vk::Image image, uint32_t computeQueue, uint32_t graphicsQueue,
    vk::AccessFlags2 computeAccess = vk::AccessFlagBits2::eShaderWrite) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eGeneral)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcQueueFamilyIndex(computeQueue)
        .setDstQueueFamilyIndex(graphicsQueue)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(computeAccess)
        .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(vk::RemainingMipLevels)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(vk::RemainingArrayLayers));
}

/// @brief Builds an ownership release for the terrain shadow depth image (shader read only -> depth attachment
/// optimal), released by compute and acquired back by graphics. Identical in shape to the offscreen depth release; kept
/// separate so call sites remain explicit about the resource they release.
/// @param image The terrain shadow depth image to release.
/// @param computeQueue Queue family index releasing ownership (compute).
/// @param graphicsQueue Queue family index acquiring ownership back (graphics).
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeShadowDepthReleaseBarrier(vk::Image image, uint32_t computeQueue,
                                                             uint32_t graphicsQueue) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSrcQueueFamilyIndex(computeQueue)
        .setDstQueueFamilyIndex(graphicsQueue)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(1)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(1));
}

// ---------------------------------------------------------------------------------------------
// Same-queue transitions
// ---------------------------------------------------------------------------------------------

/// @brief Builds a same-queue transition for the terrain shadow depth image (depth attachment optimal -> shader read
/// only) on a single queue family.
/// @param image The terrain shadow depth image to transition.
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeShadowDepthSameQueueAcquireBarrier(vk::Image image) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                         vk::PipelineStageFlagBits2::eLateFragmentTests)
        .setSrcAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(1)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(1));
}

/// @brief Builds a same-queue transition for the transmittance map (general -> transfer destination), so the reset
/// clear / upload can write to it.
/// @param image The transmittance map image to transition.
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeTransmittanceMapToTransferDstBarrier(vk::Image image) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eGeneral)
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(vk::RemainingMipLevels)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(vk::RemainingArrayLayers));
}

/// @brief Builds a same-queue transition for the transmittance map (transfer destination ->general), performed after
/// the reset clear so compute can write the map again.
/// @param image The transmittance map image to transition.
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeTransmittanceMapToGeneralBarrier(vk::Image image) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(vk::RemainingMipLevels)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(vk::RemainingArrayLayers));
}

/// @brief Builds a same-queue transition for the transmittance map (compute write -> sampled read), so the color pass
/// can sample the map written by the transmittance pass. This is not a queue-family transfer.
/// @param image The transmittance map image to transition.
/// @return The configured image memory barrier.
inline vk::ImageMemoryBarrier2 MakeTransmittanceMapToShaderReadBarrier(vk::Image image) noexcept
{
    return vk::ImageMemoryBarrier2()
        .setImage(image)
        .setOldLayout(vk::ImageLayout::eGeneral)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSubresourceRange(vk::ImageSubresourceRange()
                                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                 .setBaseMipLevel(0)
                                 .setLevelCount(vk::RemainingMipLevels)
                                 .setBaseArrayLayer(0)
                                 .setLayerCount(vk::RemainingArrayLayers));
}
} // namespace render_system::fog
