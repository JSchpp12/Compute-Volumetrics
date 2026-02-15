#pragma once

#include "TerrainShapeInfo.hpp"

#include <string>

class TerrainShapeInfoLoader
{
  public:
    explicit TerrainShapeInfoLoader(std::string shapeFilePath); 

    TerrainShapeInfo load() const;

  private:
    std::string m_shapeFilePath; 
};