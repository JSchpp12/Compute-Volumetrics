#include "VolumeDataBase.hpp"

#include "FileHelpers.hpp"

#include <nanovdb/tools/CreateNanoGrid.h>

void VolumeDataBase::prep(){
    auto baseGrid = LoadVDBBaseGrid(m_pathToFile); 
    auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

    convertVolumeFormat(grid);
    m_nanoVDB = LoadNanoVDB(grid);
}

void VolumeDataBase::writeDataToBuffer(star::StarBuffers::Buffer &buffer) const{
    void *mapped = nullptr;
    buffer.map(&mapped);

    buffer.writeToBuffer(m_nanoVDB->data(), mapped, m_nanoVDB->bufferSize());

    buffer.unmap();
}

openvdb::GridBase::Ptr VolumeDataBase::LoadVDBBaseGrid(const std::string &pathToFile){
    if (!star::file_helpers::FileExists(pathToFile))
    {
        std::ostringstream oss;
        oss << "Provided vdb file does not exist: " << pathToFile;
        throw std::runtime_error(oss.str());
    }

    openvdb::initialize();

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

std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> VolumeDataBase::LoadNanoVDB(openvdb::SharedPtr<openvdb::FloatGrid> &grid){
    return std::make_unique<nanovdb::GridHandle<nanovdb::HostBuffer>>(nanovdb::tools::createNanoGrid(*grid));
}