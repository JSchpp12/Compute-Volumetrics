#include "FogControlInfo.hpp"

FogControlInfoTransfer::FogControlInfoTransfer(const float &fogNearDist, const float &fogFarDist)
    : fogNearDist(fogNearDist), fogFarDist(fogFarDist)
{
}

std::unique_ptr<star::StarBuffer> FogControlInfoTransfer::createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const{
    auto create = star::StarBuffer::BufferCreationArgs(
        sizeof(float), 2, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_AUTO, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eConcurrent,
        "FogControlBuffer_SRC");

    return std::make_unique<star::StarBuffer>(allocator, create); 
}

std::unique_ptr<star::StarBuffer> FogControlInfoTransfer::createFinal(vk::Device &device, VmaAllocator &allocator) const{
    auto create = star::StarBuffer::BufferCreationArgs(
        sizeof(float), 2, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VMA_MEMORY_USAGE_AUTO, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eConcurrent,
        "FogControlBuffer");

    return std::make_unique<star::StarBuffer>(allocator, create); 
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

std::unique_ptr<star::TransferRequest::Buffer> FogControlInfoController::createTransferRequest(
    const vk::PhysicalDevice &physicalDevice)
{
    this->lastFogFarDist = this->currentFogFarDist;
    this->lastFogNearDist = this->currentFogNearDist;

    return std::make_unique<FogControlInfoTransfer>(this->currentFogNearDist, this->currentFogFarDist);
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