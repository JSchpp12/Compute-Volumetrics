#pragma once

#include "VolumeDataBase.hpp"

class LevelSetData : public VolumeDataBase
{
  public:
    LevelSetData(std::string pathToFile, openvdb::GridClass gridClass)
        : VolumeDataBase(std::move(pathToFile), std::move(gridClass))
    {
    }

    virtual ~LevelSetData() = default; 

  private:

  virtual void convertVolumeFormat(openvdb::SharedPtr<openvdb::FloatGrid> &baseGrid) const override; 
};
