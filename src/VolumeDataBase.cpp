#include "VolumeDataBase.hpp"

#include "FileHelpers.hpp"

#include <starlight/core/Exceptions.hpp>

#include <nanovdb/io/IO.h>
#include <nanovdb/tools/CreateNanoGrid.h>
#include <nanovdb/tools/NanoToOpenVDB.h>

#include <cassert>

void VolumeDataBase::prep()
{
    if (m_pathToFile.extension() == ".vdb")
    {
        m_nanoVDB = prepVDBFile();
    }

    m_nanoVDB = prepNVDBFile();
}

void VolumeDataBase::writeDataToBuffer(star::StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    buffer.writeToBuffer(m_nanoVDB->data(), mapped, m_nanoVDB->bufferSize());

    buffer.unmap();
}

std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> VolumeDataBase::prepVDBFile() const
{
    auto baseGrid = LoadVDBBaseGrid(m_pathToFile.string());
    auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

    convertVolumeFormat(grid);
    return LoadNanoVDB(grid);
}

std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> VolumeDataBase::prepNVDBFile() const
{
    auto nano = LoadNanoVDB(m_pathToFile.string());

    // convert to openvdb and convert as needed
    auto vdb = nanovdb::tools::nanoToOpenVDB(*nano);
    auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(vdb);
    convertVolumeFormat(grid);
    return LoadNanoVDB(grid);
}

std::array<glm::vec3, 2> VolumeDataBase::getAABB() const
{
    assert(m_nanoVDB && "NanoVDB structure was never prepared");

    const auto indexBBox = m_nanoVDB->grid<float>()->tree().bbox();

    return {glm::vec3{indexBBox.min().x(), indexBBox.min().y(), indexBBox.min().z()},
            glm::vec3{indexBBox.max().x(), indexBBox.max().y(), indexBBox.max().z()}};
}

openvdb::GridBase::Ptr VolumeDataBase::LoadVDBBaseGrid(const std::string &pathToFile)
{
    if (!star::file_helpers::FileExists(pathToFile))
    {
        std::ostringstream oss;
        oss << "Provided vdb file does not exist: " << pathToFile;
        STAR_THROW(oss.str());
    }

    openvdb::io::File file(pathToFile);
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

    return baseGrid;
}

std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> VolumeDataBase::LoadNanoVDB(
    openvdb::SharedPtr<openvdb::FloatGrid> &grid)
{
    return std::make_unique<nanovdb::GridHandle<nanovdb::HostBuffer>>(nanovdb::tools::createNanoGrid(*grid));
}

std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> VolumeDataBase::LoadNanoVDB(const std::string &path)
{
    return std::make_unique<nanovdb::GridHandle<nanovdb::HostBuffer>>(nanovdb::io::readGrid(path));
}