#pragma once

#include "StarBuffers/Buffer.hpp"

#include <openvdb/Types.h>
#include <nanovdb/GridHandle.h>
#include <nanovdb/HostBuffer.h>

#include <string>
#include <cassert>

class VolumeData{
public:
    VolumeData(std::string pathToFile, openvdb::GridClass gridClass) 
    : m_pathToFile(std::move(pathToFile)), m_requestedGridType(std::move(gridClass)) {
        m_nanoVDB = loadNanoVDB(); 
    }

    void writeDataToBuffer(star::StarBuffers::Buffer &buffer) const; 

    size_t getSize() const{
        return m_nanoVDB->gridSize(); 
    }

private:
    std::string m_pathToFile;
    openvdb::GridClass m_requestedGridType; 
    std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> m_nanoVDB;

    std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> loadNanoVDB() const; 

    static bool CheckFileExists(const std::string &pathToFile); 
};