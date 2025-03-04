#pragma once

#include "Handle.hpp"
#include "ManagerBuffer.hpp"
#include "ObjVertInfo.hpp"
#include "ObjIndicesInfo.hpp"
#include "StarBuffer.hpp"
#include "StarMesh.hpp"
#include "FileHelpers.hpp"
#include "Vertex.hpp"
#include "FileTexture.hpp"
#include "MathHelpers.hpp"
#include "TextureMaterial.hpp"
#include "StarDevice.hpp"

#include <tbb/tbb.h>
#include <glm/glm.hpp>
#include <gdal_priv.h>

#include <string>
#include <memory>


class TerrainChunk {
public:
	TerrainChunk(const std::string& heightFile, const std::string& textureFile,
		const glm::vec2& upperLeft, const glm::vec2& lowerRight)
		: heightFile(heightFile), textureFile(textureFile), upperLeft(upperLeft), lowerRight(lowerRight) {
		verifyFiles();
	};

	/// @brief Load meshes from the provided files
	void load(); 

	std::unique_ptr<star::StarMesh> getMesh(){
		auto texture = std::make_shared<star::FileTexture>(this->textureFile);
		auto material = std::make_shared<star::TextureMaterial>(texture);

		star::Handle vertBuffer = star::ManagerBuffer::addRequest(std::make_unique<star::ObjVertInfo>(*verts));
		star::Handle indBuffer = star::ManagerBuffer::addRequest(std::make_unique<star::ObjIndicesInfo>(*inds));
		return std::make_unique<star::StarMesh>(vertBuffer, indBuffer, *verts, *inds, material, false); 
	};

	std::string& getTextureFile();

	star::StarBuffer& getIndexBuffer(){
		assert(this->indBuffer && "Index buffer has not been initialized. Make sure to call load() first.");
		return *this->indBuffer;
	}

	star::StarBuffer& getVertexBuffer(){
		assert(this->vertBuffer && "Vertex buffer has not been initialized. Make sure to call load() first.");
		return *this->vertBuffer; 
	}

private:
	std::unique_ptr<std::vector<star::Vertex>> verts;
	std::unique_ptr<std::vector<uint32_t>> inds;
	std::unique_ptr<star::StarBuffer> indBuffer, vertBuffer;
	std::unique_ptr<star::StarMesh> mesh;
	std::string heightFile, textureFile; 
	glm::vec2 upperLeft, lowerRight;

	void verifyFiles() const; 

	static std::unique_ptr<star::StarBuffer> createVertBuffer(star::StarDevice& device, std::vector<star::Vertex>& verts); 

	static std::unique_ptr<star::StarBuffer> createIndexBuffer(star::StarDevice& device, std::vector<uint32_t>& inds);

	/// @brief Extract height info from the file and calculate ver
	/// @param dataset GDALDataset to use
	/// @param terrainCenter will be updated by function with the calculated terrain center
	/// @return 
	static void loadLocation(GDALDataset *const dataset, const glm::vec2& upperLeft, const glm::vec2& lowerRight, std::vector<star::Vertex>& verts, glm::vec3& terrainCenter);

	static void loadInds(GDALDataset* const dataset, std::vector<uint32_t>& inds);

	static void calculateNormals(std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds);
	/// @brief Update all vert locations to be centered around the terrain center
	/// @param terrainCenter center of the terrain
	/// @param verts all of the vertices to update
	static void centerAroundTerrainOrigin(const glm::vec3& terrainCenter, std::vector<star::Vertex>& verts);

	void loadGeomInfo(GDALDataset *const dataset, std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds) const; 
};

struct TerrainChunkProcessor {
	TerrainChunkProcessor(TerrainChunk chunks[]) 
		: chunks(chunks) {};

	void operator()(const tbb::blocked_range<size_t>& r) const {
		TerrainChunk* localChunks = this->chunks; 

		for (size_t i = r.begin(); i != r.end(); ++i) {
			localChunks[i].load(); 
		}
	}

private:
	TerrainChunk* const chunks; 
};