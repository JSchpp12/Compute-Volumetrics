#pragma once

#include <filesystem>
#include <memory>

class VDBFileInfo
{
  public:
    explicit VDBFileInfo(std::filesystem::path file) : m_file(std::move(file))
    {
    }
    virtual ~VDBFileInfo() = default;

  protected:
    std::filesystem::path m_file;
};