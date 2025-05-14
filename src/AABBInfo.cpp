#include "AABBInfo.hpp"


std::unique_ptr<star::StarBuffer> AABBTransfer::createStagingBuffer(vk::Device& device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
    const vk::DeviceSize alignmentSize = star::StarBuffer::GetAlignment(sizeof(glm::vec4), this->minUniformBufferOffsetAlignment); 

    return star::StarBuffer::Builder(allocator)
		.setAllocationCreateInfo(
			star::Allocator::AllocationBuilder()
				.setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
				.setUsage(VMA_MEMORY_USAGE_AUTO)
				.build(),
			vk::BufferCreateInfo()
				.setSharingMode(vk::SharingMode::eExclusive)
				.setSize(star::CastHelpers::size_t_to_unsigned_int(2 * alignmentSize))
				.setUsage(vk::BufferUsageFlagBits::eTransferSrc),
			"AABBInfo_SRC"
		)
		.setInstanceCount(2)
		.setInstanceSize(sizeof(glm::mat4))
		.setMinOffsetAlignment(this->minUniformBufferOffsetAlignment)
		.build();
}

std::unique_ptr<star::StarBuffer> AABBTransfer::createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
    const std::vector<uint32_t> indices = {
        this->computeQueueFamilyIndex,
        transferQueueFamilyIndex
    };

    const vk::DeviceSize alignmentSize = star::StarBuffer::GetAlignment(sizeof(glm::vec4), this->minUniformBufferOffsetAlignment); 

    return star::StarBuffer::Builder(allocator)
		.setAllocationCreateInfo(
			star::Allocator::AllocationBuilder()
				.setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
				.setUsage(VMA_MEMORY_USAGE_AUTO)
				.build(),
			vk::BufferCreateInfo()
				.setSharingMode(vk::SharingMode::eConcurrent)
				.setQueueFamilyIndexCount(2)
				.setQueueFamilyIndices(indices)
				.setSize(2 * alignmentSize)
				.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
			"AABBInfo"
		)
		.setInstanceCount(2)
		.setInstanceSize(sizeof(glm::mat4))
		.setMinOffsetAlignment(this->minUniformBufferOffsetAlignment)
        .build();
}

void AABBTransfer::writeDataToStageBuffer(star::StarBuffer &buffer) const
{
    buffer.map();

    std::array<glm::vec4, 2> rawaabbBounds = this->aabbBounds;
    for (int i = 0; i < 2; i++){
        buffer.writeToIndex(&rawaabbBounds[i], i); 
    }

    buffer.unmap();
}

std::unique_ptr<star::TransferRequest::Buffer> AABBController::createTransferRequest(
    star::StarDevice &device)
{
    return std::make_unique<AABBTransfer>(
        this->aabbBounds,
        device.getQueueFamily(star::Queue_Type::Tcompute).getQueueFamilyIndex(),
        device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment
    );
}
