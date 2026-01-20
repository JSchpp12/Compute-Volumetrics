#include "AABBTransfer.hpp"

std::unique_ptr<star::StarBuffers::Buffer> AABBTransfer::createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const
{
    return star::StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            star::Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(star::common::helper::size_t_to_unsigned_int(2 * sizeof(glm::mat4)))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "AABBInfo_SRC")
        .setInstanceCount(2)
        .setInstanceSize(sizeof(glm::mat4))
        .buildUnique();
}

std::unique_ptr<star::StarBuffers::Buffer> AABBTransfer::createFinal(vk::Device &device, VmaAllocator &allocator,
                                                            const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {
        this->computeQueueFamilyIndex,
    };

    for (const auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    return star::StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            star::Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(indices.size())
                .setPQueueFamilyIndices(indices.data())
                .setSize(2 * sizeof(glm::mat4))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "AABBInfo")
        .setInstanceCount(2)
        .setInstanceSize(sizeof(glm::mat4))
        .buildUnique();
}

void AABBTransfer::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    std::array<glm::vec4, 2> rawaabbBounds = this->aabbBounds;
    for (size_t i{0}; i < 2; i++)
    {
        buffer.writeToIndex(&rawaabbBounds[i], mapped, i);
    }

    buffer.unmap();
}