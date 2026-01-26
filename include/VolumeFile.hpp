#pragma once

#include <filesystem>

class VolumeFile
{
  public:
    VolumeFile() = default;
    VolumeFile(std::filesystem::path compressedSrcFile, std::filesystem::path dataFilePath);

    void prepDataFile() const;

    bool doesDataFileExist() const;

    const std::filesystem::path &getDataFilePath() const
    {
        return m_dataFilePath;
    }
    const std::filesystem::path &getCompressedSrcFilePath() const
    {
        return m_compressedSrcFile;
    }

  private:
    std::filesystem::path m_compressedSrcFile;
    std::filesystem::path m_dataFilePath;

    void decompressDataFile() const;
};