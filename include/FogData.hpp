#pragma once

#include "StarBuffers/Buffer.hpp"

#include "VolumeDataBase.hpp"

#include <cassert>
#include <string>


class FogData : public VolumeDataBase
{
  public:
    FogData(std::string pathToFile, openvdb::GridClass gridClass)
        : VolumeDataBase(std::move(pathToFile), std::move(gridClass))
    {
    }

  private:
    virtual void convertVolumeFormat(openvdb::SharedPtr<openvdb::FloatGrid> &baseGrid) const override;
};