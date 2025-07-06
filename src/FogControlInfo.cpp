#include "FogControlInfo.hpp"

FogControlInfoTransfer::FogControlInfoTransfer(const FogInfo::FinalizedInfo &fogInfo,
                                               const uint32_t &computeQueueFamilyIndex)
    : fogInfo(fogInfo), computeQueueFamilyIndex(computeQueueFamilyIndex)
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
                .setSize(sizeof(FogInfo::FinalizedInfo))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "FogControlInfo_Src")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(FogInfo::FinalizedInfo))
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
                .setSize(sizeof(FogInfo::FinalizedInfo))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "FogControlInfo")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(FogInfo::FinalizedInfo))
        .build();
}

void FogControlInfoTransfer::writeDataToStageBuffer(star::StarBuffer &buffer) const
{
    buffer.map();

    {
        FogInfo::FinalizedInfo info = FogInfo::FinalizedInfo(this->fogInfo);
        buffer.writeToBuffer(&info, sizeof(FogInfo::FinalizedInfo));
    }

    buffer.unmap();
}

FogControlInfoController::FogControlInfoController(const uint8_t &getFrameInFlightIndexToUpdateOn,
                                                   const std::shared_ptr<FogInfo> currentFogInfo)
    : star::ManagerController::RenderResource::Buffer(getFrameInFlightIndexToUpdateOn), currentFogInfo(currentFogInfo)
{
}

std::unique_ptr<star::TransferRequest::Buffer> FogControlInfoController::createTransferRequest(star::StarDevice &device)
{
    this->lastFogInfo = *this->currentFogInfo;

    return std::make_unique<FogControlInfoTransfer>(
        this->currentFogInfo->getInfo(), device.getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex());
}

bool FogControlInfoController::isValid(const uint8_t &currentFrameInFlightIndex) const
{
    if (!this->star::ManagerController::RenderResource::Buffer::isValid(currentFrameInFlightIndex) &&
        *this->currentFogInfo != lastFogInfo)
    {
        return false;
    }

    return true;
}