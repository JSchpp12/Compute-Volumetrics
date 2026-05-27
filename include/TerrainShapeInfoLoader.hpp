#pragma once

#include <star_terrain/file_data/coverage_info/CoverageInfo.hpp>

#include <starlight/core/CommandBus.hpp>

#include <string>
#include <future>

class TerrainShapeInfoLoader
{
  public:
    static std::future<star::terrain::CoverageInfo> SubmitForRead(std::filesystem::path filePath,
                                                        const star::core::CommandBus &cmdBus); 

    int operator()(const std::filesystem::path &filePath); 

    std::future<star::terrain::CoverageInfo> getFuture()
    {
        return m_shapeInfo.get_future();
    } 


  private:
    std::promise<star::terrain::CoverageInfo> m_shapeInfo;

    star::terrain::CoverageInfo load(const std::string &filePath) const;
};