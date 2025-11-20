#include "Terrain.hpp"

#include "FileHelpers.hpp"
#include "TerrainGrid.hpp"

std::unordered_map<star::Shader_Stage, star::StarShader> Terrain::getShaders()
{
    std::unordered_map<star::Shader_Stage, star::StarShader> shaders;
    std::string vertShaderPath =
        star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/default.vert";
    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(
        star::Shader_Stage::vertex, star::StarShader(vertShaderPath, star::Shader_Stage::vertex)));

    std::string fragShaderPath =
        star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/basicTexture/basicTexture.frag";
    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(
        star::Shader_Stage::fragment, star::StarShader(fragShaderPath, star::Shader_Stage::fragment)));

    return shaders;
}

std::vector<std::unique_ptr<star::StarMesh>> Terrain::loadMeshes(star::core::device::DeviceContext &context){
    TerrainInfoFile fileInfo = TerrainInfoFile(m_terrainDefFile);
    const auto terrainPath = star::file_helpers::GetParentDirectory(m_terrainDefFile).value();

    TerrainGrid grid = TerrainGrid();

    std::vector<TerrainChunk> chunks;
    // make sure gdal init is setup before multi-thread init
    GDALAllRegister();

    std::set<std::string> alreadyProcessed = std::set<std::string>();
    bool setWorldCenter = false;
    glm::dvec3 worldCenter = glm::dvec3();

    const auto fullHeightFilePath = terrainPath / boost::filesystem::path(fileInfo.getFullHeightFilePath());
    for (size_t i = 0; i < fileInfo.infos().size(); i++)
    {
        const auto infoPath = terrainPath / fileInfo.infos()[i].textureFile; 
        if (!setWorldCenter)
        {
            setWorldCenter = true;

            float height = TerrainChunk::getCenterHeightFromGDAL(fullHeightFilePath.string(),
                                                                 glm::dvec3{});

            worldCenter = glm::dvec3{fileInfo.infos()[i].cornerNW.x, fileInfo.infos()[i].cornerNW.y, height};
        }

        chunks.emplace_back(fullHeightFilePath.string(),
                            infoPath.string(), fileInfo.infos()[i].cornerNE,
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

    for (size_t i = 0; i < chunks.size(); i++){
        terrainMeshes[i] = chunks[i].getMesh(context, m_meshMaterials[i]);    
    }

    return terrainMeshes; 
}

std::vector<std::shared_ptr<star::StarMaterial>> Terrain::LoadMaterials(std::string terrainInfoFile){
    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "terrains/";

    TerrainInfoFile fileInfo = TerrainInfoFile(terrainInfoFile);
    std::vector<std::shared_ptr<star::StarMaterial>> materials = std::vector<std::shared_ptr<star::StarMaterial>>(fileInfo.infos().size());

    for (size_t i = 0; i < fileInfo.infos().size(); i++){
        auto foundTexture = star::file_helpers::FindFileInDirectoryWithSameNameIgnoreFileType(mediaDirectoryPath, fileInfo.infos()[i].textureFile);
        if (!foundTexture.has_value()){
            throw std::runtime_error("Failed to find matching texture for file"); 
        }
        
        materials[i] = std::make_shared<star::TextureMaterial>(foundTexture.value()); 
    }

    return materials; 
}
