#include "VolumeData.hpp"

#include "FileHelpers.hpp"

#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/tools/FastSweeping.h>
#include <nanovdb/NanoVDB.h>
#include <nanovdb/tools/CreateNanoGrid.h>

std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> VolumeData::loadNanoVDB() const{
    if (!star::file_helpers::FileExists(m_pathToFile))
    {
        std::ostringstream oss;
        oss << "Provided vdb file does not exist: " << m_pathToFile;
        throw std::runtime_error(oss.str());
    }

    std::cout << "Beginning OpenVDB to NanoVDB conversion" << std::endl;

    openvdb::initialize();

    openvdb::io::File file(m_pathToFile);
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
    if (baseGrid->getGridClass() == openvdb::GridClass::GRID_STAGGERED || baseGrid->getGridClass() == openvdb::GridClass::GRID_UNKNOWN){
        throw std::runtime_error("Unsupported grid class"); 
    }

    if (baseGrid->getGridClass() != m_requestedGridType)
    {
        switch(m_requestedGridType){
            case(openvdb::GridClass::GRID_FOG_VOLUME):
            {
                std::cout << "Converting to fog" << std::endl;
                openvdb::tools::sdfToFogVolume(*grid);
                std::cout << "Done" << std::endl;
                break;
            }
            case(openvdb::GridClass::GRID_LEVEL_SET):
            {
                // std::cout << "WARNING: Converting from fog to level set is experimental" << std::endl;
                // std::cout << "Converting to level set" << std::endl;
                // openvdb::tools::fogToSdf(*grid, 1.0);
                // std::cout << "Done" << std::endl;
                // break;
            }
            default:
                throw std::runtime_error("Unsupported requested fog type");
        }
    }

    // auto srcGrid = openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>(100.0f, openvdb::Vec3f(0.0f), 1.0f);
    // Convert the OpenVDB grid, srcGrid, into a NanoVDB grid handle.
    auto handle = nanovdb::tools::createNanoGrid(*grid);

    std::cout << "NanoVDB Ready" << std::endl;
    return std::make_unique<nanovdb::GridHandle<nanovdb::HostBuffer>>(nanovdb::tools::createNanoGrid(*grid));
}

void VolumeData::writeDataToBuffer(star::StarBuffers::Buffer &buffer) const{
    void *mapped = nullptr;
    buffer.map(&mapped);

    buffer.writeToBuffer(m_nanoVDB->data(), mapped, m_nanoVDB->bufferSize());

    buffer.unmap();
}

bool VolumeData::CheckFileExists(const std::string &pathToFile){
    return star::file_helpers::FileExists(pathToFile); 
}