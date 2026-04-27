#pragma once

#include "render_system/fog/struct/DispatchIndirectCommand.hpp"
#include "render_system/fog/struct/RayMarchState.hpp"

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/wrappers/graphics/StarBuffers/Buffer.hpp>

namespace render_system::fog
{

// static inline star::StarBuffers::Buffer CreateRayCountBuffer(star::core::device::DeviceContext &ctx,
//                                                              std::string debugName)
//{
//     const vk::DeviceSize size = sizeof(RayCountInfo);
//
//     return star::StarBuffers::Buffer::Builder(ctx.getDevice().getAllocator().get())
//         .setAllocationCreateInfo(
//             DefaultCreateInfo(),
//             vk::BufferCreateInfo()
//                 .setSize(size)
//                 .setSharingMode(vk::SharingMode::eExclusive)
//                 .setUsage(vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer),
//             std::move(debugName))
//         .setInstanceCount(1)
//         .setInstanceSize(size)
//         .build();
// };

static inline star::StarBuffers::Buffer CreateActiveRayStorageBuffer(star::core::device::DeviceContext &ctx,
                                                                     std::string debugName,
                                                                     const vk::Extent2D &screenDimensions)
{
    vk::DeviceSize size = sizeof(RayMarchState);
    {
        const vk::DeviceSize instanceCount = screenDimensions.width * screenDimensions.height;
        const vk::DeviceSize instanceSize = sizeof(RayMarchState);
        const vk::DeviceSize headerSize = sizeof(ActiveRayHeader);
        size = instanceSize * instanceCount + headerSize;
    }
    const vk::DeviceSize instanceCount = screenDimensions.width * screenDimensions.height;

    return star::StarBuffers::Buffer::Builder(ctx.getDevice().getAllocator().get())
        .setAllocationCreateInfo(star::Allocator::AllocationBuilder()
                                     .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                     .setUsage(VMA_MEMORY_USAGE_AUTO)
                                     .build(),
                                 vk::BufferCreateInfo()
                                     .setSize(size)
                                     .setSharingMode(vk::SharingMode::eExclusive)
                                     .setUsage(vk::BufferUsageFlagBits::eStorageBuffer |
                                               vk::BufferUsageFlagBits::eIndirectBuffer |
                                               vk::BufferUsageFlagBits::eTransferDst),
                                 std::move(debugName))
        .setInstanceCount(1)
        .setInstanceSize(size)
        .build();
};
} // namespace render_system::fog