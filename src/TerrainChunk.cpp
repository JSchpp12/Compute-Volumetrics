#include "TerrainChunk.hpp"

#include "TextureMaterial.hpp"
#include "FileHelpers.hpp"
#include "Vertex.hpp"
#include "MathHelpers.hpp"

#include "Handle.hpp"
#include "ManagerRenderResource.hpp"
#include "ManagerController_RenderResource_TextureFile.hpp"
#include "ManagerController_RenderResource_VertInfo.hpp"
#include "ManagerController_RenderResource_IndicesInfo.hpp"


TerrainChunk::TerrainChunk(const std::string& heightFile, const std::string& textureFile, const glm::dvec2& upperLeft, const glm::dvec2& lowerRight, const glm::dvec3& offset) : heightFile(heightFile), textureFile(textureFile), upperLeft(upperLeft), lowerRight(lowerRight), offset(offset){
	verifyFiles(); 
}

void TerrainChunk::verifyFiles() const
{
	if (!star::FileHelpers::FileExists(this->heightFile))
		throw std::runtime_error("Height file does not exist: " + this->heightFile);

	if (!star::FileHelpers::FileExists(this->textureFile))
		throw std::runtime_error("Texture file does not exist: " + this->textureFile); 
}

double TerrainChunk::getCenterHeightFromGDAL(const std::string& geoTiff){
	GDALDataset* dataset = (GDALDataset*)GDALOpen(geoTiff.c_str(), GA_ReadOnly);

	if (dataset == NULL) {
		throw std::runtime_error("Failed to create dataset");
	}

	float* line = nullptr;

	GDALRasterBand* band = dataset->GetRasterBand(1);
	int nXSize = band->GetXSize();
	int nYSize = band->GetYSize();
	line = (float*)CPLMalloc(sizeof(float) * nXSize * nYSize);
	band->RasterIO(GF_Read, 0, 0, nXSize, nYSize, line, nXSize, nYSize, GDT_Float32, 0, 0);

	double result = MathHelpers::feetToMeters(line[(nYSize / 2) * nXSize + (nXSize / 2)]);

	CPLFree(line); 
	GDALClose(dataset);

	return result; 
}

void TerrainChunk::load() {
	this->verts = std::make_unique<std::vector<star::Vertex>>();
	this->inds = std::make_unique<std::vector<uint32_t>>();

	GDALDataset* dataset = (GDALDataset*)GDALOpen(this->heightFile.c_str(), GA_ReadOnly);

	if (dataset == NULL) {
		throw std::runtime_error("Failed to create dataset");
	}

	loadGeomInfo(dataset, *this->verts, *this->inds);

	GDALClose(dataset);
}

std::unique_ptr<star::StarMesh> TerrainChunk::getMesh(){
	auto material = std::make_shared<star::TextureMaterial>(star::ManagerRenderResource::addRequest(std::make_unique<star::ManagerController::RenderResource::TextureFile>(this->textureFile)));

	star::Handle vertBuffer = star::ManagerRenderResource::addRequest(std::make_unique<star::ManagerController::RenderResource::VertInfo>(*verts));
	star::Handle indBuffer = star::ManagerRenderResource::addRequest(std::make_unique<star::ManagerController::RenderResource::IndicesInfo>(*inds));
	return std::make_unique<star::StarMesh>(vertBuffer, indBuffer, *verts, *inds, material, false); 
}

std::string& TerrainChunk::getTextureFile()
{
	return this->textureFile;
}


void TerrainChunk::loadLocation(GDALDataset* const dataset, const glm::vec2& upperLeft, const glm::vec2& lowerRight, std::vector<glm::dvec3>& vertPositions, std::vector<glm::vec2>& vertTextureCoords, glm::dvec3& terrainCenter, const glm::vec2& offset) {
	float* line = nullptr;

	GDALRasterBand* band = dataset->GetRasterBand(1);

	int nXSize = band->GetXSize();
	int nYSize = band->GetYSize();

	double xTexStep = 1.0f / nXSize;
	double yTexStep = 1.0f / nYSize;

	double vert = (glm::abs(upperLeft.x) - glm::abs(lowerRight.x)) / nXSize;
	double horz = (glm::abs(lowerRight.y) - glm::abs(upperLeft.y)) / nYSize;

	line = (float*)CPLMalloc(sizeof(float) * nXSize * nYSize);
	band->RasterIO(GF_Read, 0, 0, nXSize, nYSize, line, nXSize, nYSize, GDT_Float32, 0, 0);

	//calculate locations
	for (int i = 0; i < nYSize; i++) {
		for (int j = 0; j < nXSize; j++) {
			auto location = glm::dvec3{
				(upperLeft.x + j * vert),
				(upperLeft.y + i * horz),
				line[i * nXSize + j],
			};

			vertPositions.push_back(location);
			vertTextureCoords.push_back(glm::vec2(j * xTexStep, i * yTexStep));
		}
	}

	CPLFree(line);
}

void TerrainChunk::loadInds(GDALDataset* const dataset, std::vector<uint32_t>& inds) {
	GDALRasterBand* band = dataset->GetRasterBand(1); 

	int nXSize = band->GetXSize();
	int nYSize = band->GetYSize();

	uint32_t indexCounter = 0;

	for (int i = 0; i < nYSize; i++) {
		for (int j = 0; j < nXSize; j++) {
			if (j % 2 == 1 && i % 2 == 1 && i != nYSize - 1 && j != nXSize - 1) {
				//this is a 'central' vert where drawing should be based around
				// 
				//uppper left
				uint32_t center = indexCounter;
				uint32_t centerLeft = indexCounter - 1;
				uint32_t centerRight = indexCounter + 1;
				uint32_t upperLeft = indexCounter - 1 - nXSize;
				uint32_t upperCenter = indexCounter - nXSize;
				uint32_t upperRight = indexCounter - nXSize + 1;
				uint32_t lowerLeft = indexCounter + nXSize - 1;
				uint32_t lowerCenter = indexCounter + nXSize;
				uint32_t lowerRight = indexCounter + nXSize + 1;
				//1
				inds.push_back(center);
				inds.push_back(upperLeft);
				inds.push_back(centerLeft);
				//2
				inds.push_back(center);
				inds.push_back(upperCenter);
				inds.push_back(upperLeft);

				if (i != i - 1 && j == j - 1)
				{
					//side piece
					//cant do 3,4,5,6,
					//7
					inds.push_back(center);
					inds.push_back(lowerLeft);
					inds.push_back(lowerCenter);
					//8
					inds.push_back(center);
					inds.push_back(centerLeft);
					inds.push_back(lowerLeft);

				}
				else if (i == i - 1 && j != j - 1)
				{
					//bottom piece
					//cant do 5,6,7,8
					//3
					inds.push_back(center);
					inds.push_back(upperRight);
					inds.push_back(upperCenter);
					//4
					inds.push_back(center);
					inds.push_back(centerRight);
					inds.push_back(upperRight);
				}
				else if (i != i - 1 && j != j - 1) {
					//3
					inds.push_back(center);
					inds.push_back(upperRight);
					inds.push_back(upperCenter);
					//4
					inds.push_back(center);
					inds.push_back(centerRight);
					inds.push_back(upperRight);
					//5
					inds.push_back(center);
					inds.push_back(lowerRight);
					inds.push_back(centerRight);
					//6
					inds.push_back(center);
					inds.push_back(lowerCenter);
					inds.push_back(lowerRight);
					//7
					inds.push_back(center);
					inds.push_back(lowerLeft);
					inds.push_back(lowerCenter);
					//8
					inds.push_back(center);
					inds.push_back(centerLeft);
					inds.push_back(lowerLeft);
				}

			}
			indexCounter++;
		}
	}
}

void TerrainChunk::calculateNormals(std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds) {
	//calculate normals
	for (int i = 0; i < inds.size(); i += 3) {
		auto& vert1 = verts.at(inds.at(i));
		auto& vert2 = verts.at(inds.at(i + 1));
		auto& vert3 = verts.at(inds.at(i + 2));

		glm::vec3 edge1 = vert2.pos - vert1.pos;
		glm::vec3 edge2 = vert3.pos - vert1.pos;
		glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

		vert1.normal += normal;
		vert2.normal += normal;
		vert3.normal += normal;
	}
}

void TerrainChunk::centerAroundTerrainOrigin(const glm::dvec3& terrainCenter, std::vector<glm::dvec3>& vertPositions, const glm::dvec3& worldCenterLatLon) const {
	//set all verts around origin

	// auto worldCenterECEF = MathHelpers::toECEF(vertPositions[0].x, vertPositions[0].y, vertPositions[0].z); 
	// auto worldCenterToENUTransformation = MathHelpers::getECEFToENUTransformation(vertPositions[0].x, vertPositions[0].y);

	// auto worldCenterECEF = MathHelpers::toECEF(terrainCenter.x, terrainCenter.y,terrainCenter.z); 
	// auto worldCenterToENUTransformation = MathHelpers::getECEFToENUTransformation(terrainCenter.x, terrainCenter.y);

	const glm::dvec3 worldCenterECEF = MathHelpers::toECEF(worldCenterLatLon.x, worldCenterLatLon.y, MathHelpers::feetToMeters(worldCenterLatLon.z)); 
	const auto worldCenterToENUTransformation = MathHelpers::getECEFToENUTransformation(worldCenterLatLon.x, worldCenterLatLon.y);

	for (int i = 0; i < vertPositions.size(); i++) {
		// const glm::dvec3 vertECEF = MathHelpers::toECEF(vertPositions[i].x, vertPositions[i].y, vertPositions[i].z); 

		// const glm::dvec3 displaced = vertECEF - testCenterECEF;
		// const glm::vec3 resultPosition = testCenterToENUTransformation * displaced; 
		// vertPositions[i] = glm::vec3{resultPosition.y, resultPosition.z, resultPosition.x};

		const glm::dvec3 vertECEF = MathHelpers::toECEF(vertPositions[i].x, vertPositions[i].y, vertPositions[i].z); 
		const glm::dvec3 displacedECEF = vertECEF - worldCenterECEF; 
		const glm::dvec3 result = worldCenterToENUTransformation * displacedECEF; 
		vertPositions[i] = glm::vec3{result.x, result.z, result.y}; 

		// const glm::dvec3 displacedECEF =  vertECEF - testCenterECEF; 

		// vertPositions[i] = worldCenterToENUTransformation * displacedECEF; 
	}
}

void TerrainChunk::loadGeomInfo(GDALDataset *const dataset, std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds) const {
	glm::dvec3 terrainCenter = glm::dvec3(
		(this->upperLeft.x + this->lowerRight.x) / 2.0f,
		(this->upperLeft.y + this->lowerRight.y) / 2.0f, 
		getCenterHeightFromGDAL(this->heightFile)); 
	
	std::vector<glm::dvec3> rawVertPositionCoords;
	std::vector<glm::vec2> vertTextureCoords; 
	loadLocation(dataset, this->upperLeft, this->lowerRight, rawVertPositionCoords, vertTextureCoords, terrainCenter, this->offset);

	loadInds(dataset, inds); 
	centerAroundTerrainOrigin(terrainCenter, rawVertPositionCoords, this->offset);

	for (int i = 0; i < rawVertPositionCoords.size(); i++){
		verts.push_back(star::Vertex(rawVertPositionCoords.at(i), {}, {}, vertTextureCoords.at(i))); 
	}

	calculateNormals(verts, inds);
}