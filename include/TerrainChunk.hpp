#pragma once

#include "StarBuffer.hpp"
#include "StarMesh.hpp"
#include "StarDevice.hpp"

#include <tbb/tbb.h>
#include <glm/glm.hpp>
#include <gdal_priv.h>

#include <string>
#include <memory>

class TerrainChunk {
public:
	TerrainChunk(const std::string& heightFile, const std::string& textureFile,
		const glm::dvec2& upperLeft, const glm::dvec2& lowerRight, const glm::dvec3& offset);

	/// @brief Load meshes from the provided files
	void load(); 

	std::unique_ptr<star::StarMesh> getMesh();

	std::string& getTextureFile();

	star::StarBuffer& getIndexBuffer(){
		assert(this->indBuffer && "Index buffer has not been initialized. Make sure to call load() first.");
		return *this->indBuffer;
	}

	star::StarBuffer& getVertexBuffer(){
		assert(this->vertBuffer && "Vertex buffer has not been initialized. Make sure to call load() first.");
		return *this->vertBuffer; 
	}

	static double getCenterHeightFromGDAL(const std::string& geoTiff); 

private:
	std::unique_ptr<std::vector<star::Vertex>> verts;
	std::unique_ptr<std::vector<uint32_t>> inds;
	std::unique_ptr<star::StarBuffer> indBuffer, vertBuffer;
	std::unique_ptr<star::StarMesh> mesh;
	std::string heightFile, textureFile; 
	const glm::dvec2 upperLeft, lowerRight;
	const glm::dvec3 offset; 

	void verifyFiles() const; 

	/// @brief Extract height info from the file and calculate ver
	/// @param dataset GDALDataset to use
	/// @param terrainCenter will be updated by function with the calculated terrain center
	/// @return 
	static void loadLocation(GDALDataset *const dataset, const glm::vec2& upperLeft, 
		const glm::vec2& lowerRight, std::vector<glm::dvec3>& vertPositions, std::vector<glm::vec2>& vertTextureCoords, 
		glm::dvec3& terrainCenter, const glm::vec2& offset);

	static void loadInds(GDALDataset* const dataset, std::vector<uint32_t>& inds);

	static void calculateNormals(std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds);
	/// @brief Update all vert locations to be centered around the terrain center
	/// @param terrainCenter center of the terrain
	/// @param verts all of the vertices to update
	void centerAroundTerrainOrigin(const glm::dvec3& terrainCenter, std::vector<glm::dvec3>& vertPositions, const glm::dvec3& worldCenterLatLon) const;

	// static void applyOffset(const glm::vec2& offset, std::vector<star::Vertex>& verts); 

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