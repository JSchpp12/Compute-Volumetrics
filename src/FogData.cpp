#include "FogData.hpp"

#include "FileHelpers.hpp"

#include <openvdb/tools/FastSweeping.h>

void FogData::convertVolumeFormat(openvdb::SharedPtr<openvdb::FloatGrid> &baseGrid) const
{
    std::cout << "Beginning OpenVDB to NanoVDB conversion" << std::endl;

    
    if (baseGrid->getGridClass() == openvdb::GridClass::GRID_STAGGERED ||
        baseGrid->getGridClass() == openvdb::GridClass::GRID_UNKNOWN)
    {
        throw std::runtime_error("Unsupported grid class");
    }

    if (baseGrid->getGridClass() != m_requestedGridClass)
    {
        if (m_requestedGridClass == openvdb::GridClass::GRID_LEVEL_SET)
        {
            std::cout << "WARNING: Converting from fog to level set is experimental" << std::endl;
            std::cout << "Converting to level set" << std::endl;
            openvdb::tools::fogToSdf(*baseGrid, 1.0);
            std::cout << "Done" << std::endl;
        }
        else
        {
            throw std::runtime_error("Unsupported conversion required");
        }
    }
}