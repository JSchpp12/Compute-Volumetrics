#pragma once

#include "StarObject.hpp"

#include <filesystem>

class Terrain : public star::StarObject
{
  public:
    Terrain(star::core::device::DeviceContext &context, std::string terrainDefFile)
        : star::StarObject(LoadMaterials(terrainDefFile)), m_terrainDefFile(std::move(terrainDefFile))
    {
    };

    std::filesystem::path getHeightInfoFilePath() const
    {
        return std::filesystem::path(m_terrainDefFile) / "height_info.json";
    }
    std::filesystem::path getShapeFilePath() const
    {
        return std::filesystem::path(m_terrainDefFile) / "Shape.json";
    }

  protected:
    std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

    std::vector<std::unique_ptr<star::StarMesh>> loadMeshes(star::core::device::DeviceContext &context) override;

  private:
    std::string m_terrainDefFile;

    static std::vector<std::shared_ptr<star::StarMaterial>> LoadMaterials(const std::string &terrainDir);
};