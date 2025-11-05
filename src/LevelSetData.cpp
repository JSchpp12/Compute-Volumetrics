#include "LevelSetData.hpp"

#include <openvdb/openvdb.h>
#include <openvdb/tools/LevelSetUtil.h>

void LevelSetData::convertVolumeFormat(openvdb::SharedPtr<openvdb::FloatGrid> &baseGrid) const
{
    auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
    if (baseGrid->getGridClass() == openvdb::GridClass::GRID_STAGGERED ||
        baseGrid->getGridClass() == openvdb::GridClass::GRID_UNKNOWN)
    {
        throw std::runtime_error("Unsupported grid class");
    }

    if (baseGrid->getGridClass() != m_requestedGridClass)
    {
        if (m_requestedGridClass == openvdb::GridClass::GRID_FOG_VOLUME)
        {
            std::cout << "Converting to fog" << std::endl;
            openvdb::tools::sdfToFogVolume(*grid);
            std::cout << "Done" << std::endl;
        }
        else
        {
            throw std::runtime_error("Cant convert");
        }
    } 
}
