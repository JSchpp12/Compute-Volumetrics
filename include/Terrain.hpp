#pragma once

#include "StarObject.hpp"
#include "TextureMaterial.hpp"
#include "TerrainChunk.hpp"
#include "TerrainInfoFile.hpp"
#include "MathHelpers.hpp"

#include <gdal_priv.h>
#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>
#include <tbb/tbb.h>

#include "nlohmann/json.hpp"

class Terrain : public star::StarObject{
public:
	Terrain(const std::string& terrainDefFile)
		: terrainDefFile(terrainDefFile){
			loadGeometry();
		};

protected:
	std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

	void loadGeometry();

private: 
	const std::string terrainDefFile;
};