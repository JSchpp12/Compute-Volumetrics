#include "LevelSetData.hpp"

#include <openvdb/openvdb.h>
#include <openvdb/tools/LevelSetUtil.h>

#include <starlight/core/Exceptions.hpp>

void LevelSetData::convertVolumeFormat(openvdb::SharedPtr<openvdb::FloatGrid> &baseGrid) const
{
    auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
    if (baseGrid->getGridClass() == openvdb::GridClass::GRID_STAGGERED ||
        baseGrid->getGridClass() == openvdb::GridClass::GRID_UNKNOWN)
    {
        STAR_THROW("Unsupported grid class type");
    }

    const auto gridClass = baseGrid->getGridClass();
    if (gridClass != m_requestedGridClass)
    {
        if (m_requestedGridClass == openvdb::GridClass::GRID_FOG_VOLUME)
        {
            std::cout << "Converting to fog" << std::endl;
            openvdb::tools::sdfToFogVolume(*grid);
            std::cout << "Done" << std::endl;
        }
        else
        {
            STAR_THROW("Failed to convert SDF to fog body");
        }
    }
}
