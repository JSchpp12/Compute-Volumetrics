#pragma once

#include "StarBuffers/Buffer.hpp"

#include <nanovdb/GridHandle.h>
#include <nanovdb/HostBuffer.h>

#include <openvdb/Grid.h>
#include <openvdb/Types.h>
#include <openvdb/openvdb.h>

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <filesystem>


class VolumeDataBase
{
  public:
    VolumeDataBase(std::filesystem::path pathToFile, openvdb::GridClass gridClass)
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

    std::array<glm::vec3, 2> getAABB() const;

    /// The grid's WORLD-space AABB (voxel size applied). This is the volume's bounds in the object's
    /// local space; the world bounds come from transforming it by the instance display matrix.
    std::array<glm::vec3, 2> getWorldAABB() const;

    static openvdb::GridBase::Ptr LoadVDBBaseGrid(const std::string &pathToVDBFile);

  protected:
    std::filesystem::path m_pathToFile;
    openvdb::GridClass m_requestedGridClass;
    std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> m_nanoVDB = nullptr; 

    virtual void convertVolumeFormat(openvdb::SharedPtr<openvdb::FloatGrid> &grid) const = 0;

    std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> prepVDBFile() const;
    std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> prepNVDBFile() const;

  private:
    static std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> LoadNanoVDB(openvdb::SharedPtr<openvdb::FloatGrid> &grid); 
    static std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> LoadNanoVDB(const std::string &path); 


};