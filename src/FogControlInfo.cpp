#include "FogControlInfo.hpp"

#include <starlight/core/helper/queue/QueueHelpers.hpp>

std::unique_ptr<star::StarBuffers::Buffer> FogControlInfoTransfer::createStagingBuffer(vk::Device &device,
                                                                                       VmaAllocator &allocator) const
{
    const vk::DeviceSize size = sizeof(FogInfo::FinalizedInfo); 

    return star::StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            star::Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(size)
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "FogControlInfo_Src")
        .setInstanceCount(1)
        .setInstanceSize(size)
        .buildUnique();
}

std::unique_ptr<star::StarBuffers::Buffer> FogControlInfoTransfer::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {this->computeQueueFamilyIndex};
    for (const auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    const vk::DeviceSize size = sizeof(FogInfo::FinalizedInfo);

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
                .setSize(size)
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "FogControlInfo")
        .setInstanceCount(1)
        .setInstanceSize(size)
        .buildUnique();
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

void FogInfoController::prepRender(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    star::ManagerController::RenderResource::Buffer::prepRender(context, numFramesInFlight);

    m_lastFogInfo.resize(numFramesInFlight);
}

std::unique_ptr<star::TransferRequest::Buffer> FogInfoController::createTransferRequest(
    star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    m_lastFogInfo[frameInFlightIndex] = *m_currentFogInfo;

    const auto &defaultQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tcompute)
            ->getParentQueueFamilyIndex();

    return std::make_unique<FogControlInfoTransfer>(m_currentFogInfo->getInfo(), defaultQueueFamilyIndex);
}

bool FogInfoController::doesFrameInFlightDataNeedUpdated(const uint8_t &currentFrameInFlightIndex) const
{
    return *m_currentFogInfo != m_lastFogInfo[currentFrameInFlightIndex];
}