#pragma once

#include <gdal_priv.h>
#include <tbb/tbb.h>
#include <vma/vk_mem_alloc.h>

#include <glm/glm.hpp>

#include "MathHelpers.hpp"
#include "StarObject.hpp"
#include "TerrainChunk.hpp"
#include "TerrainInfoFile.hpp"
#include "TextureMaterial.hpp"
#include "nlohmann/json.hpp"

class Terrain : public star::StarObject
{
  public:
    Terrain(star::core::device::DeviceContext &context, const std::string &terrainDefFile) : terrainDefFile(terrainDefFile)
    {
        loadGeometry(context);
    };

  protected:
    std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

    void loadGeometry(star::core::device::DeviceContext &context);

  private:
    const std::string terrainDefFile;
};