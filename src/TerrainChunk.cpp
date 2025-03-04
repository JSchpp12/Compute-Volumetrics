#include "TerrainChunk.hpp"

void TerrainChunk::verifyFiles() const
{
	if (!star::FileHelpers::FileExists(this->heightFile))
		throw std::runtime_error("Height file does not exist: " + this->heightFile);

	if (!star::FileHelpers::FileExists(this->textureFile))
		throw std::runtime_error("Texture file does not exist: " + this->textureFile); 
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

std::string& TerrainChunk::getTextureFile()
{
	return this->textureFile;
}


void TerrainChunk::loadLocation(GDALDataset* const dataset, const glm::vec2& upperLeft, const glm::vec2& lowerRight, std::vector<star::Vertex>& verts, glm::vec3& terrainCenter) {
	float* line = nullptr;

	GDALRasterBand* band = dataset->GetRasterBand(1);

	int nXSize = band->GetXSize();
	int nYSize = band->GetYSize();

	float xTexStep = 1.0f / nXSize;
	float yTexStep = 1.0f / nYSize;

	double vert = upperLeft.x - lowerRight.y;
	double horz = lowerRight.y - upperLeft.y;

	line = (float*)CPLMalloc(sizeof(float) * nXSize * nYSize);
	band->RasterIO(GF_Read, 0, 0, nXSize, nYSize, line, nXSize, nYSize, GDT_Float32, 0, 0);


	//calculate locations
	for (int i = 0; i < nYSize; i++) {
		for (int j = 0; j < nXSize; j++) {
			auto location = MathHelpers::toECEF(glm::vec3{
				upperLeft.x - j * vert / nXSize,
				upperLeft.y + i * horz / nYSize,
				line[i * nXSize + j]
			});

			terrainCenter = (terrainCenter + location) / glm::vec3(2.0f, 2.0f, 2.0f);
			glm::vec2 texCoord = glm::vec2(j * xTexStep, i * yTexStep);
			verts.push_back(star::Vertex(location, glm::vec3(), glm::vec3(), texCoord));
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

void TerrainChunk::centerAroundTerrainOrigin(const glm::vec3& terrainCenter, std::vector<star::Vertex>& verts) {
	//set all verts around origin 
	for (auto& vert : verts) {
		vert.pos = vert.pos - terrainCenter;
		vert.normal = glm::normalize(vert.normal);
	}
}

void TerrainChunk::loadGeomInfo(GDALDataset *const dataset, std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds) const {
	glm::vec3 terrainCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	loadLocation(dataset, this->upperLeft, this->lowerRight, verts, terrainCenter);
	loadInds(dataset, inds); 

	calculateNormals(verts, inds);
	centerAroundTerrainOrigin(terrainCenter, verts);
}