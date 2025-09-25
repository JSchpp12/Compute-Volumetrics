#include "VDBInfo.hpp"

#include "FileHelpers.hpp"

std::unique_ptr<star::StarBuffers::Buffer> VDBRequest::createStagingBuffer(vk::Device &device,
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
                .setSize(m_volumeData.getSize())
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "VDBInfo_SRC")
        .setInstanceCount(1)
        .setInstanceSize(m_volumeData.getSize())
        .build();
}

std::unique_ptr<star::StarBuffers::Buffer> VDBRequest::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    uint32_t numInds = 1;
    std::vector<uint32_t> indices = {m_computeQueueIndex};
    for (const auto &index : transferQueueFamilyIndex)
    {
        indices.emplace_back(index);
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
                .setSize(m_volumeData.getSize())
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer),
            "VDBInfo")
        .setInstanceCount(1)
        .setInstanceSize(m_volumeData.getSize())
        .build();
}

void VDBRequest::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    m_volumeData.writeDataToBuffer(buffer);
}

std::unique_ptr<star::TransferRequest::Buffer> VDBInfoController::createTransferRequest(
    star::core::device::StarDevice &device)
{
    return std::make_unique<VDBRequest>(device.getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex(),
                                        m_vdbPath, m_ensureThisType);
}
