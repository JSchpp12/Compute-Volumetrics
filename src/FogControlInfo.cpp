#include "FogControlInfo.hpp"

FogControlInfoTransfer::FogControlInfoTransfer(const float &fogNearDist, const float &fogFarDist,
                                               const uint32_t &computeQueueFamilyIndex,
                                               const vk::DeviceSize &minUniformBufferOffsetAlignment)
    : fogNearDist(fogNearDist), fogFarDist(fogFarDist), computeQueueFamilyIndex(computeQueueFamilyIndex),
      minUniformBufferOffsetAlignment(minUniformBufferOffsetAlignment)
{
}

std::unique_ptr<star::StarBuffer> FogControlInfoTransfer::createStagingBuffer(vk::Device &device,
                                                                              VmaAllocator &allocator) const
{
    return star::StarBuffer::Builder(allocator)
        .setAllocationCreateInfo(
            star::Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(sizeof(float) * 2)
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "FogControlInfo_Src")
        .setInstanceCount(2)
        .setInstanceSize(sizeof(float))
        .build();
}

std::unique_ptr<star::StarBuffer> FogControlInfoTransfer::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
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
                .setSize(sizeof(float) * 2)
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "FogControlInfo")
        .setInstanceCount(2)
        .setInstanceSize(sizeof(float))
        .build();
}

void FogControlInfoTransfer::writeDataToStageBuffer(star::StarBuffer &buffer) const
{
    buffer.map();

    {
        float copier = float(this->fogNearDist);
        buffer.writeToIndex(&copier, 0);
        copier = float(this->fogFarDist);
        buffer.writeToIndex(&copier, 1);
    }

    buffer.unmap();
}

FogControlInfoController::FogControlInfoController(const uint8_t &frameInFlightIndexToUpdateOn,
                                                   const float &currentFogNearDist, const float &currentFogFarDist)
    : star::ManagerController::RenderResource::Buffer(frameInFlightIndexToUpdateOn),
      currentFogFarDist(currentFogFarDist), currentFogNearDist(currentFogNearDist)
{
}

std::unique_ptr<star::TransferRequest::Buffer> FogControlInfoController::createTransferRequest(star::StarDevice &device)
{
    this->lastFogFarDist = this->currentFogFarDist;
    this->lastFogNearDist = this->currentFogNearDist;

    return std::make_unique<FogControlInfoTransfer>(
        this->currentFogNearDist, this->currentFogFarDist,
        device.getQueueFamily(star::Queue_Type::Tcompute).getQueueFamilyIndex(),
        device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment);
}

bool FogControlInfoController::isValid(const uint8_t &currentFrameInFlightIndex) const
{
    if (!this->star::ManagerController::RenderResource::Buffer::isValid(currentFrameInFlightIndex) &&
        (this->lastFogNearDist != this->currentFogNearDist || this->lastFogFarDist != this->currentFogFarDist))
    {
        return false;
    }

    return true;
}