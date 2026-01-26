#pragma once

#include "StarBuffers/Buffer.hpp"

#include <nanovdb/GridHandle.h>
#include <nanovdb/HostBuffer.h>

#include <openvdb/Grid.h>
#include <openvdb/Types.h>
#include <openvdb/openvdb.h>

#include <memory>
#include <string>


class VolumeDataBase
{
  public:
    VolumeDataBase(std::string pathToFile, openvdb::GridClass gridClass)
        : m_pathToFile(std::move(pathToFile)), m_requestedGridClass(std::move(gridClass))
    {
    }

    virtual ~VolumeDataBase() = default;

    void prep();

    void writeDataToBuffer(star::StarBuffers::Buffer &buffer) const;

    size_t getSize() const{
        assert(m_nanoVDB && "NanoVDB needs to be prepared before accessed"); 

        return m_nanoVDB->gridSize();
    }

    static openvdb::GridBase::Ptr LoadVDBBaseGrid(const std::string &pathToVDBFile);

  protected:
    std::string m_pathToFile;
    openvdb::GridClass m_requestedGridClass;
    std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> m_nanoVDB = nullptr; 

    virtual void convertVolumeFormat(openvdb::SharedPtr<openvdb::FloatGrid> &grid) const = 0;

  private:
    static std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> LoadNanoVDB(openvdb::SharedPtr<openvdb::FloatGrid> &grid); 
};