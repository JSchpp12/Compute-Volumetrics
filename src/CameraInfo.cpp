#include "CameraInfo.hpp"

std::unique_ptr<star::StarBuffer> CameraInfo::createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const
{
    return star::StarBuffer::Builder(allocator)
        .setAllocationCreateInfo(
            star::Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(sizeof(CameraData))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "CameraInfo_Src")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(CameraData))
        .build();
}

std::unique_ptr<star::StarBuffer> CameraInfo::createFinal(vk::Device &device, VmaAllocator &allocator,
                                                          const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {this->computeQueueFamilyIndex};
	for (const auto &index : transferQueueFamilyIndex)
		indices.push_back(index);

    return star::StarBuffer::Builder(allocator)
        .setAllocationCreateInfo(
            star::Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(indices.size())
                .setPQueueFamilyIndices(indices.data())
                .setSize(sizeof(CameraData))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "CameraInfo")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(CameraData))
        .build();
}

void CameraInfo::writeDataToStageBuffer(star::StarBuffer &buffer) const
{
    buffer.map();

    auto data = CameraData{glm::inverse(camera.getProjectionMatrix()),
                           glm::vec2(camera.getResolution()),
                           camera.getResolution().x / camera.getResolution().y,
                           camera.getFarClippingDistance(),
                           camera.getNearClippingDistance(),
                           tan(camera.getVerticalFieldOfView(true))};

    buffer.writeToIndex(&data, 0);

    buffer.unmap();
}

std::unique_ptr<star::TransferRequest::Buffer> CameraInfoController::createTransferRequest(star::StarDevice &device)
{
    return std::make_unique<CameraInfo>(
        this->camera, device.getQueueFamily(star::Queue_Type::Tcompute).getQueueFamilyIndex(),
        device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment);
}