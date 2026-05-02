#pragma once

#include "TerrainShapeInfo.hpp"

#include <starlight/core/CommandBus.hpp>

#include <string>
#include <future>

class TerrainShapeInfoLoader
{
  public:
    static std::future<TerrainShapeInfo> SubmitForRead(std::filesystem::path filePath,
                                                        const star::core::CommandBus &cmdBus); 

    int operator()(const std::filesystem::path &filePath); 

    std::future<TerrainShapeInfo> getFuture()
    {
        return m_shapeInfo.get_future();
    } 


  private:
    std::promise<TerrainShapeInfo> m_shapeInfo;

    TerrainShapeInfo load(const std::string &filePath) const;
};