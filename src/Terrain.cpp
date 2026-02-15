#include "Terrain.hpp"

#include "TerrainGrid.hpp"
#include "TerrainShapeInfoLoader.hpp"

#include <starlight/common/ConfigFile.hpp>
#include <starlight/common/helpers/FileHelpers.hpp>

std::unordered_map<star::Shader_Stage, star::StarShader> Terrain::getShaders()
{
    std::unordered_map<star::Shader_Stage, star::StarShader> shaders;
    std::string vertShaderPath =
        star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/default.vert";
    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(
        star::Shader_Stage::vertex, star::StarShader(vertShaderPath, star::Shader_Stage::vertex)));

    std::string fragShaderPath =
        star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/default.frag";
    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(
        star::Shader_Stage::fragment, star::StarShader(fragShaderPath, star::Shader_Stage::fragment)));

    return shaders;
}

std::vector<std::unique_ptr<star::StarMesh>> Terrain::loadMeshes(star::core::device::DeviceContext &context)
{
    TerrainInfoFile fileInfo = TerrainInfoFile(m_terrainDefFile);
    const auto terrainPath = star::file_helpers::GetParentDirectory(m_terrainDefFile).value();

    TerrainShapeInfo shapeInfo;
    {
        const auto terrainShapeFile = boost::filesystem::path(terrainPath) / "Shape.json";
        auto loader = TerrainShapeInfoLoader(terrainShapeFile.string());

        shapeInfo = loader.load();
    }

    TerrainGrid grid = TerrainGrid();

    std::vector<TerrainChunk> chunks;
    // make sure gdal init is setup before multi-thread init
    GDALAllRegister();

    std::set<std::string> alreadyProcessed = std::set<std::string>();
    bool setWorldCenter = false;
    glm::dvec3 worldCenter(shapeInfo.center.x, shapeInfo.center.y, 0);

    const auto fullHeightFilePath = terrainPath / boost::filesystem::path(fileInfo.getFullHeightFilePath());
    for (size_t i = 0; i < fileInfo.infos().size(); i++)
    {
        const auto infoPath = terrainPath / fileInfo.infos()[i].textureFile;
        if (!setWorldCenter)
        {
            setWorldCenter = true;

            float height = TerrainChunk::GetCenterHeightFromGDAL(fullHeightFilePath.string());
            worldCenter.z = height; 
        }

        chunks.emplace_back(fullHeightFilePath.string(), infoPath.string(), fileInfo.infos()[i].cornerNE,
                            fileInfo.infos()[i].cornerSE, fileInfo.infos()[i].cornerSW, fileInfo.infos()[i].cornerNW,
                            worldCenter, fileInfo.infos()[i].center);
    }

    // parallel load meshes
    std::cout << "Launching load tasks" << std::endl;
    TerrainChunkProcessor chunkProcessor = TerrainChunkProcessor(chunks.data());
    oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, chunks.size()), chunkProcessor);
    std::cout << "Done" << std::endl;

    auto terrainMeshes = std::vector<std::unique_ptr<star::StarMesh>>(chunks.size());
    assert(chunks.size() == m_meshMaterials.size() && "Every chunk should have its own material");

    for (size_t i = 0; i < chunks.size(); i++)
    {
        terrainMeshes[i] = chunks[i].getMesh(context, m_meshMaterials[i]);
    }

    return terrainMeshes;
}

std::vector<std::shared_ptr<star::StarMaterial>> Terrain::LoadMaterials(std::string terrainInfoFile)
{
    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "terrains/";

    TerrainInfoFile fileInfo = TerrainInfoFile(terrainInfoFile);
    std::vector<std::shared_ptr<star::StarMaterial>> materials =
        std::vector<std::shared_ptr<star::StarMaterial>>(fileInfo.infos().size());

    for (size_t i = 0; i < fileInfo.infos().size(); i++)
    {
        const boost::filesystem::path *found = nullptr;
        auto files = star::file_helpers::FindFilesInDirectoryWithSameNameIgnoreFileType(
            mediaDirectoryPath, fileInfo.infos()[i].textureFile);
        for (const auto &file : files)
        {
            if (file.extension() == ".ktx2")
            {
                found = &file;
            }
        }

        if (found == nullptr)
        {
            STAR_THROW("Failed to find matching texture for file");
        }

        materials[i] = std::make_shared<star::TextureMaterial>(found->string());
    }

    return materials;
}
