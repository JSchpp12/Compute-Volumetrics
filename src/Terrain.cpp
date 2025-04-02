#include "Terrain.hpp"

#include "TerrainGrid.hpp"
#include "FileHelpers.hpp"

std::unordered_map<star::Shader_Stage, star::StarShader> Terrain::getShaders()
{
	std::unordered_map<star::Shader_Stage, star::StarShader> shaders; 
	std::string vertShaderPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/default.vert";
	shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(star::Shader_Stage::vertex, star::StarShader(vertShaderPath, star::Shader_Stage::vertex)));

	std::string fragShaderPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/basicTexture/basicTexture.frag";
	shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(star::Shader_Stage::fragment, star::StarShader(fragShaderPath, star::Shader_Stage::fragment)));

	return shaders; 
}

void Terrain::loadGeometry()
{
	TerrainInfoFile fileInfo = TerrainInfoFile(this->terrainDefFile); 
	const std::string terrainPath = star::FileHelpers::GetBaseFileDirectory(this->terrainDefFile); 

	TerrainGrid grid = TerrainGrid(); 
	
	std::set<std::string> targets = {
		"s2045440",
		"s2050445",
		"s2040435"
	};

	std::vector<TerrainChunk> chunks; 
	//make sure gdal init is setup before multi-thread init
	GDALAllRegister();
	
	std::set<std::string> alreadyProcessed = std::set<std::string>(); 
	bool setWorldCenter = false; 
	glm::dvec3 worldCenter = glm::dvec3(); 

	for (int i = 0; i < fileInfo.infos().size(); i++){
		std::string fileName = star::FileHelpers::GetFileNameWithoutExtension(fileInfo.infos()[i].heightFile);
		fileName = fileName.substr(0, fileName.find("_geo"));
		//there are duplicates in json -- MUST FIX
		if (alreadyProcessed.find(fileName) == alreadyProcessed.end()){
			alreadyProcessed.insert(fileName);

			
			if (!setWorldCenter){
				setWorldCenter = true; 
				const std::string path = terrainPath + "/" + fileInfo.infos()[i].heightFile;
				float height = TerrainChunk::getCenterHeightFromGDAL(path); 

				worldCenter = glm::dvec3{
					fileInfo.infos()[i].cornerSE.x,
					fileInfo.infos()[i].cornerSE.y, 
					height};
			}

			chunks.push_back(TerrainChunk{
				terrainPath + "/" + fileInfo.infos()[i].heightFile, 
				terrainPath + "/" + fileInfo.infos()[i].textureFile,
				fileInfo.infos()[i].cornerNW,
				fileInfo.infos()[i].cornerSE,
				worldCenter
			});

			// grid.add(fileInfo.infos()[i].heightFile, 
			// 	fileInfo.infos()[i].textureFile,
			// 	fileInfo.infos()[i].cornerNW,
			// 	fileInfo.infos()[i].cornerSE);
		}
	}

	// auto test = grid.getFinalizedChunks(); 


	//parallel load meshes
	std::cout << "Launching load tasks" << std::endl;
	TerrainChunkProcessor chunkProcessor = TerrainChunkProcessor(chunks.data());
	oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, chunks.size()), chunkProcessor);
	std::cout << "Done" << std::endl;

	for (auto& chunk : chunks){
		this->meshes.emplace_back(chunk.getMesh());
	}
}
