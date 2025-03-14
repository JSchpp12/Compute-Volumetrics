#include "Terrain.hpp"

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

	std::vector<TerrainChunk> chunks = std::vector<TerrainChunk>();
	for (int i = 0; i < fileInfo.infos().size(); i++){

		if (i == 5)
			break;
			
		chunks.push_back(TerrainChunk{
			fileInfo.infos()[i].heightFile, 
			fileInfo.infos()[i].surfaceTexture,
			fileInfo.infos()[i].upperLeft,
			fileInfo.infos()[i].lowerRight
		});
	}

	//make sure gdal init is setup before multi-thread init
	GDALAllRegister();

	//parallel load meshes
	std::cout << "Launching load tasks" << std::endl;
	TerrainChunkProcessor chunkProcessor = TerrainChunkProcessor(chunks.data());
	oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, chunks.size()), chunkProcessor);
	std::cout << "Done" << std::endl;

	for (auto& chunk : chunks){
		this->meshes.emplace_back(chunk.getMesh());
	}
}
