#pragma once

#include "VDBTransfer.hpp"
#include "VolumeFile.hpp"

#include <filesystem>
#include <vector>

class VolumeDirectoryProcessor
{
  public:
    VolumeDirectoryProcessor(std::filesystem::path dataDir, std::filesystem::path tmpWorkingDir);

    void init();

    std::vector<VDBTransfer> createAllGPUTransfers();

    const std::vector<VolumeFile> &getProcessedFiles() const
    {
        return m_processedFiles;    
    };

    size_t getNumProcessedFiles()
    {
        return m_processedFiles.size();
    }

  protected:
    std::filesystem::path m_dataDir;
    std::filesystem::path m_tmpWorkingDir;
    std::vector<VolumeFile> m_processedFiles;

    void decompressAllFiles();
};