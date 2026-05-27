#include "Terrain.hpp"

#include "TerrainChunk.hpp"
#include "TerrainGrid.hpp"

#include <star_terrain/struct/TerrainInfo.hpp>
#include <star_terrain/io/TerrainInfo.hpp>
#include "TerrainShapeInfoLoader.hpp"

#include <starlight/common/ConfigFile.hpp>
#include <starlight/common/helpers/FileHelpers.hpp>
#include <starlight/common/materials/TextureMaterial.hpp>

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
    const auto infoPath = getHeightInfoFilePath();
    auto [readResult, fileInfo] = star::terrain::io::ReadTerrainInfo(infoPath.string()); 

    const auto terrainPath = std::filesystem::path(m_terrainDefFile);
    const auto terrainShapeFile = getShapeFilePath();
    auto loadingShapeInfo = TerrainShapeInfoLoader::SubmitForRead(getShapeFilePath(), context.getCmdBus());
    TerrainGrid grid = TerrainGrid();

    std::vector<TerrainChunk> chunks;
    // make sure gdal init is setup before multi-thread init
    GDALAllRegister();

    std::set<std::string> alreadyProcessed = std::set<std::string>();
    bool setWorldCenter = false;

    TerrainShapeInfo shapeInfo = loadingShapeInfo.get();
    glm::dvec3 worldCenter(shapeInfo.center.x, shapeInfo.center.y, 0);

    const auto fullHeightFilePath = terrainPath / std::filesystem::path(fileInfo.fullHeightFilePath);
    for (size_t i = 0; i < fileInfo.chunks.size(); i++)
    {
        const auto infoPath = terrainPath / fileInfo.chunks[i].textureFile;
        if (!setWorldCenter)
        {
            setWorldCenter = true;

            double height = TerrainChunk::GetHeightAtLocationFromGDAL(fullHeightFilePath.string(), shapeInfo.center.x,
                                                                      shapeInfo.center.y)
                                .value();
            worldCenter.z = height;
        }

        chunks.emplace_back(fullHeightFilePath.string(), infoPath.string(), fileInfo.chunks[i].cornerNE,
                            fileInfo.chunks[i].cornerSE, fileInfo.chunks[i].cornerSW, fileInfo.chunks[i].cornerNW,
                            worldCenter, fileInfo.chunks[i].center);
    }
    star::core::logging::info("Launching load tasks");
    tbb::enumerable_thread_specific<std::unique_ptr<ThreadLocalDataset>> tls;

    tbb::parallel_for(tbb::blocked_range<size_t>(0, chunks.size()), [&](const tbb::blocked_range<size_t> &r) {
        auto &local = tls.local();

        if (!local)
            local = std::make_unique<ThreadLocalDataset>(fullHeightFilePath.string());

        for (size_t i = r.begin(); i != r.end(); ++i)
        {
            chunks[i].load(local->ds);
        }
    });
    tls.clear();
    star::core::logging::info("Done");

    auto terrainMeshes = std::vector<std::unique_ptr<star::StarMesh>>(chunks.size());
    assert(chunks.size() == m_meshMaterials.size() && "Every chunk should have its own material");

    for (size_t i = 0; i < chunks.size(); i++)
    {
        terrainMeshes[i] = chunks[i].getMesh(context, m_meshMaterials[i]);
    }

    return terrainMeshes;
}

std::vector<std::shared_ptr<star::StarMaterial>> Terrain::LoadMaterials(const std::string &terrainDir)
{
    const auto terrainDirPath = std::filesystem::path(terrainDir);

    auto [readResult, fileInfo] = star::terrain::io::ReadTerrainInfo((terrainDirPath / "height_info.json").string());
    std::vector<std::shared_ptr<star::StarMaterial>> materials =
        std::vector<std::shared_ptr<star::StarMaterial>>(fileInfo.chunks.size());

    for (size_t i = 0; i < fileInfo.chunks.size(); i++)
    {
        const std::filesystem::path *found = nullptr;
        auto files = star::file_helpers::FindFilesInDirectoryWithSameNameIgnoreFileType(
            terrainDir, fileInfo.chunks[i].textureFile);
        for (const auto &file : files)
        {
            if (file.extension() == ".ktx2")
            {
                found = &file;
            }
        }

        if (found == nullptr)
        {
            std::ostringstream oss;
            oss << "Failed to find matching texture for file: " << fileInfo.chunks[i].textureFile << std::endl
                << "Ensure terrains are prepared with compressed textures" << std::endl;
            STAR_THROW(oss.str());
        }

        materials[i] = std::make_shared<star::TextureMaterial>(found->string());
    }

    return materials;
}
