#include "VDBInfo.hpp"

#include <nanovdb/NanoVDB.h>
#include <nanovdb/tools/CreateNanoGrid.h> // converter from OpenVDB to NanoVDB (includes NanoVDB.h and GridManager.h)
#include <openvdb/openvdb.h>

#include "FileHelpers.hpp"

void VDBRequest::prep()
{
    m_gridData = loadNanoVolume();
}

std::unique_ptr<star::StarBuffers::Buffer> VDBRequest::createStagingBuffer(vk::Device &device,
                                                                           VmaAllocator &allocator) const
{
    assert(m_gridData != nullptr && "Grid must be loaded before this call");

    return star::StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            star::Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(m_gridData->bufferSize())
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "VDBInfo_SRC")
        .setInstanceCount(1)
        .setInstanceSize(m_gridData->bufferSize())
        .build();
}

std::unique_ptr<star::StarBuffers::Buffer> VDBRequest::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    assert(m_gridData && "Grid data should have been prepared before this");

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
                .setSize(m_gridData->bufferSize())
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer),
            "VDBInfo")
        .setInstanceCount(1)
        .setInstanceSize(m_gridData->bufferSize())
        .build();
}

void VDBRequest::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    assert(m_gridData && "Grid data should have been prepared before this");

    void *mapped = nullptr; 
    buffer.map(&mapped); 

    buffer.writeToBuffer(m_gridData->data(), mapped, m_gridData->bufferSize()); 

    buffer.unmap(); 
}

std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> VDBRequest::loadNanoVolume()
{
    if (!star::file_helpers::FileExists(m_vdbPath))
    {
        std::ostringstream oss;
        oss << "Provided vdb file does not exist: " << m_vdbPath;
        throw std::runtime_error(oss.str());
    }

    std::cout << "Beginning OpenVDB to NanoVDB conversion" << std::endl;

    openvdb::initialize();

    openvdb::io::File file(m_vdbPath);
    file.open();

    openvdb::GridBase::Ptr baseGrid;
    for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter)
    {
        std::cout << "Available Grids in file:" << std::endl;
        std::cout << nameIter.gridName() << std::endl;

        if (!baseGrid)
        {
            baseGrid = file.readGrid(nameIter.gridName());
        }
        else
        {
            std::cout << "Skipping extra grid: " << nameIter.gridName();
        }
    }
    file.close();

    auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
    // auto srcGrid = openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>(100.0f, openvdb::Vec3f(0.0f), 1.0f);
    // Convert the OpenVDB grid, srcGrid, into a NanoVDB grid handle.
    auto handle = nanovdb::tools::createNanoGrid(*grid);

    std::cout << "NanoVDB Ready" << std::endl; 
    return std::make_unique<nanovdb::GridHandle<nanovdb::HostBuffer>>(nanovdb::tools::createNanoGrid(*grid));
}

std::unique_ptr<star::TransferRequest::Buffer> VDBInfoController::createTransferRequest(
    star::core::device::StarDevice &device)
{
    return std::make_unique<VDBRequest>(m_vdbPath,
                                        device.getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex());
}
