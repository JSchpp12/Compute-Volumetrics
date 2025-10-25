#include "FogControlInfo.hpp"

std::unique_ptr<star::StarBuffers::Buffer> FogControlInfoTransfer::createStagingBuffer(vk::Device &device,
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
                .setSize(sizeof(FogInfo::FinalizedInfo))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "FogControlInfo_Src")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(FogInfo::FinalizedInfo))
        .build();
}

std::unique_ptr<star::StarBuffers::Buffer> FogControlInfoTransfer::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {this->computeQueueFamilyIndex};
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
                .setSize(sizeof(FogInfo::FinalizedInfo))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "FogControlInfo")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(FogInfo::FinalizedInfo))
        .build();
}

void FogControlInfoTransfer::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    {
        FogInfo::FinalizedInfo info = FogInfo::FinalizedInfo(this->fogInfo);
        buffer.writeToBuffer(&info, mapped, sizeof(FogInfo::FinalizedInfo));
    }

    buffer.unmap();
}

void FogInfoController::prepRender(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight){
    star::ManagerController::RenderResource::Buffer::prepRender(context, numFramesInFlight); 
    
    m_lastFogInfo.resize(numFramesInFlight); 
}

std::unique_ptr<star::TransferRequest::Buffer> FogInfoController::createTransferRequest(
    star::core::device::StarDevice &device, const uint8_t &frameInFlightIndex)
{
    m_lastFogInfo[frameInFlightIndex] = *m_currentFogInfo;

    return std::make_unique<FogControlInfoTransfer>(
        m_currentFogInfo->getInfo(),
        device.getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex());
}

bool FogInfoController::doesFrameInFlightDataNeedUpdated(const uint8_t &currentFrameInFlightIndex) const
{
    return *m_currentFogInfo != m_lastFogInfo[currentFrameInFlightIndex]; 
}