#include "VolumeFile.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

#include <zstd.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static std::vector<char> readFile(const std::string &path)
{

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
    {
        std::ostringstream oss;
        oss << "Failed to open: " << path;
        STAR_THROW(oss.str());
    }

    // Compute file size
    ifs.seekg(0, std::ios::end);
    std::streamoff end = ifs.tellg();
    if (end < 0)
    {
        STAR_THROW("tellg() failed (file size unknown)");
    }

    std::vector<char> data(static_cast<size_t>(end));
    ifs.seekg(0, std::ios::beg);

    if (!data.empty())
    {
        if (!ifs.read(data.data(), static_cast<std::streamsize>(data.size())))
        {
            STAR_THROW("read() failed (short read)");
        }
    }
    return data;
}

VolumeFile::VolumeFile(std::filesystem::path compressedSrcFile, std::filesystem::path dataFilePath)
    : m_compressedSrcFile(std::move(compressedSrcFile)), m_dataFilePath(std::move(dataFilePath))
{
    m_dataFilePath.replace_extension(".nvdb");
}

void VolumeFile::prepDataFile() const
{
    if (!std::filesystem::exists(m_compressedSrcFile))
    {
        STAR_THROW("Provided compressed file does not exist");
    }

    decompressDataFile();
}

bool VolumeFile::doesDataFileExist() const
{
    return std::filesystem::exists(m_dataFilePath);
}

void VolumeFile::decompressDataFile() const
{
    std::vector<char> compressedData = readFile(m_compressedSrcFile.string());
     size_t compressedSize = compressedData.size();
    unsigned long long const decompressedSize = ZSTD_getFrameContentSize(compressedData.data(), compressedSize);
    if (decompressedSize == ZSTD_CONTENTSIZE_ERROR || decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        throw std::runtime_error("Can't know the original size. Use streaming decompression instead.");
    }

    std::vector<char> decompressedData(decompressedSize);
    size_t const actualDecompressedSize =
        ZSTD_decompress(decompressedData.data(), decompressedSize, compressedData.data(), compressedSize);

    if (ZSTD_isError(actualDecompressedSize))
    {
        throw std::runtime_error(ZSTD_getErrorName(actualDecompressedSize));
    }


    auto outFilePath = std::filesystem::path(m_dataFilePath);
    outFilePath.replace_extension(".nvdb");

    std::ofstream outFile(outFilePath.string(), std::ios::binary);
    if (!outFile)
    {
        throw std::runtime_error("Failed to open output file for writing.");
    }

    outFile.write(decompressedData.data(), actualDecompressedSize);
    {
        std::ostringstream oss;
        oss << "Decompressed successfully to: " << outFilePath.string();
        star::core::logging::info(oss.str());
    }
}