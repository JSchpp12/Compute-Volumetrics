#pragma once

#include <gdal_priv.h>
#include <tbb/tbb.h>
#include <vma/vk_mem_alloc.h>

#include <glm/glm.hpp>

#include "MathHelpers.hpp"
#include "StarObject.hpp"
#include "TerrainInfoFile.hpp"
#include "TextureMaterial.hpp"
#include "nlohmann/json.hpp"

class Terrain : public star::StarObject
{
  public:
    Terrain(star::core::device::DeviceContext &context, std::string terrainDefFile)
        : star::StarObject(LoadMaterials(terrainDefFile)), m_terrainDefFile(std::move(terrainDefFile))
    {
    };

  protected:
    std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

    std::vector<std::unique_ptr<star::StarMesh>> loadMeshes(star::core::device::DeviceContext &context) override;

  private:
    std::string m_terrainDefFile;

    std::vector<std::shared_ptr<star::StarMaterial>> LoadMaterials(std::string terrainInfoFile);
};