#pragma once

#include "StarObject.hpp"

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

    static std::vector<std::shared_ptr<star::StarMaterial>> LoadMaterials(const std::string &terrainDir);
};