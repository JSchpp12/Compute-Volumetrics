#include "VolumeDirectoryProcessor.hpp"

#include "VolumeFile.hpp"

#include <starlight/common/ConfigFile.hpp>
#include <starlight/core/Exceptions.hpp>

#include <tbb/tbb.h>

#include <string_view>

static std::vector<std::filesystem::path> ListFiles(const std::filesystem::path &dir)
{
    std::vector<std::filesystem::path> files;

    for (const auto &entry : std::filesystem::directory_iterator(dir))
    {
        files.emplace_back(entry.path());
    }

    return files;
}

static std::vector<std::filesystem::path> GetAllFilesWithExtension(const std::filesystem::path &dir,
                                                                   std::string_view extension)
{
    std::vector<std::filesystem::path> compFiles;

    auto allFiles = ListFiles(dir);

    for (const auto &file : allFiles)
    {
        if (file.extension() == extension)
        {
            compFiles.emplace_back(file);
        }
    }

    return compFiles;
}

static std::unordered_set<std::filesystem::path> GetAllCompressedFiles(const std::filesystem::path &dir)
{
    std::unordered_set<std::filesystem::path> files;

    for (const auto &file : GetAllFilesWithExtension(dir, ".zst"))
    {
        files.insert(file);
    }

    return files;
}

static std::unordered_set<std::string> GetAllFileNamesWithoutExtension(const std::filesystem::path &dir)
{
    std::unordered_set<std::string> names;

    for (const std::filesystem::path &file : ListFiles(dir))
    {
        names.insert(file.filename().string());
    }

    return names;
}

VolumeDirectoryProcessor::VolumeDirectoryProcessor(std::filesystem::path dataDir, std::filesystem::path tmpWorkingDir)
    : m_dataDir(std::move(dataDir)), m_tmpWorkingDir(std::move(tmpWorkingDir))
{
}

std::vector<VDBTransfer> VolumeDirectoryProcessor::createAllGPUTransfers()
{
    std::vector<VDBTransfer> transfers;

    return transfers;
}

void VolumeDirectoryProcessor::init()
{
    decompressAllFiles();
}

void VolumeDirectoryProcessor::decompressAllFiles()
{
    const std::vector<std::filesystem::path> filesToDecompress = ListFiles(m_dataDir);
    m_processedFiles.resize(filesToDecompress.size());

    for (size_t i{0}; i < filesToDecompress.size(); i++)
    {
        m_processedFiles[i] = VolumeFile(filesToDecompress[i], m_tmpWorkingDir / filesToDecompress[i].filename());
    }

    star::core::logging::info("Dispatching threads to decompress all files");
    tbb::parallel_for(tbb::blocked_range<std::size_t>(0, m_processedFiles.size()),
                      [&](const tbb::blocked_range<size_t> &r) {
                          for (size_t i = r.begin(); i < r.end(); i++)
                          {
                              if (!m_processedFiles[i].doesDataFileExist())
                              {
                                  m_processedFiles[i].prepDataFile();
                              }
                          }
                      });

    star::core::logging::info("Done decompressing data files");
}