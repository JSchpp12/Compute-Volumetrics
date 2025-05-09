#include "AABBInfo.hpp"


std::unique_ptr<star::StarBuffer> AABBTransfer::createStagingBuffer(vk::Device& device, VmaAllocator &allocator) const{
    auto create = star::StarBuffer::BufferCreationArgs(sizeof(glm::vec4), 2,
                                                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                                VMA_MEMORY_USAGE_AUTO, vk::BufferUsageFlagBits::eTransferSrc,
                                                vk::SharingMode::eConcurrent, "AABBInfoBuffer_SRC");

    return std::make_unique<star::StarBuffer>(allocator, create); 
}

std::unique_ptr<star::StarBuffer> AABBTransfer::createFinal(vk::Device &device, VmaAllocator &allocator) const{
    auto create = star::StarBuffer::BufferCreationArgs(sizeof(glm::vec4), 2,
                                                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                                VMA_MEMORY_USAGE_AUTO, 
                                                vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
                                                vk::SharingMode::eConcurrent, "AABBInfoBuffer");

    return std::make_unique<star::StarBuffer>(allocator, create); 
}

void AABBTransfer::writeDataToStageBuffer(star::StarBuffer &buffer) const
{
    buffer.map();

    std::array<glm::vec4, 2> aabbBounds = this->aabbBounds;
    buffer.writeToBuffer(aabbBounds.data(), sizeof(this->aabbBounds));

    buffer.unmap();
}

std::unique_ptr<star::TransferRequest::Buffer> AABBController::createTransferRequest(
    const vk::PhysicalDevice &physicalDevice)
{
    return std::make_unique<AABBTransfer>(this->aabbBounds);
}
