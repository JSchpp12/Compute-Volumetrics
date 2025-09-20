#include "CameraInfo.hpp"

std::unique_ptr<star::StarBuffers::Buffer> CameraInfo::createStagingBuffer(vk::Device &device,
                                                                           VmaAllocator &allocator) const
{
    return star::StarBuffers::Buffer::Builder(allocator)
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

std::unique_ptr<star::StarBuffers::Buffer> CameraInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    uint32_t numInds = 1;
    std::vector<uint32_t> indices = {this->computeQueueFamilyIndex};
    for (const auto &index : transferQueueFamilyIndex)
    {
        indices.push_back(index);
        numInds++;
    }

    return star::StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            star::Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(numInds)
                .setPQueueFamilyIndices(indices.data())
                .setSize(sizeof(CameraData))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "CameraInfo")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(CameraData))
        .build();
}

void CameraInfo::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    auto data = CameraData{glm::inverse(camera->getProjectionMatrix()),
                           glm::vec2(camera->getResolution()),
                           (float)camera->getResolution().x / (float)camera->getResolution().y,
                           camera->getFarClippingDistance(),
                           camera->getNearClippingDistance(),
                           tan(camera->getVerticalFieldOfView(true))};

    buffer.writeToIndex(&data, mapped, 0);

    buffer.unmap();
}

std::unique_ptr<star::TransferRequest::Buffer> CameraInfoController::createTransferRequest(
    star::core::device::StarDevice &device)
{
    return std::make_unique<CameraInfo>(
        this->camera, device.getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex(),
        device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment);
}