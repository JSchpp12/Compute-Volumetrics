#include "FogData.hpp"

#include "FileHelpers.hpp"
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

#include <openvdb/tools/FastSweeping.h>

void FogData::convertVolumeFormat(openvdb::SharedPtr<openvdb::FloatGrid> &baseGrid) const
{
    star::core::logging::info("Beginning OpenVDB to NanoVDB conversion");

    if (baseGrid->getGridClass() == openvdb::GridClass::GRID_STAGGERED ||
        baseGrid->getGridClass() == openvdb::GridClass::GRID_UNKNOWN)
    {
        STAR_THROW("Unsupported grid class");
    }

    if (baseGrid->getGridClass() != m_requestedGridClass)
    {
        if (m_requestedGridClass == openvdb::GridClass::GRID_LEVEL_SET)
        {
            std::ostringstream oss; 
            oss << "WARNING: Converting from fog to level set is experimental" << std::endl
                << "Converting to level set" << std::endl; 
            star::core::logging::info(oss.str()); 

            openvdb::tools::fogToSdf(*baseGrid, 1.0);

            star::core::logging::info("Done"); 
        }
        else
        {
            STAR_THROW("Unsupported conversion required to convert volume to fog");
        }
    }
}