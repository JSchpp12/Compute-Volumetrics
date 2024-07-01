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

std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> Terrain::loadGeometryBuffers(star::StarDevice& device)
{
	glm::vec3 terrainCenter = glm::vec3(0.0f, 0.0f, 0.0f);

	double vert = this->upperLeft.x - this->lowerRight.x;
	double horz = this->lowerRight.y - this->upperLeft.y;

	GDALDataset* dataset = nullptr;

	GDALAllRegister();

	dataset = (GDALDataset*)GDALOpen(this->terrainDefPath.c_str(), GA_ReadOnly);

	if (dataset == NULL) {
		std::cout << "Unable to open image: " << this->terrainDefPath << std::endl;
		throw stderr;
	}

	GDALRasterBand* band = nullptr;
	band = dataset->GetRasterBand(1);
	int nXSize = band->GetXSize();
	int nYSize = band->GetYSize();

	std::unique_ptr<std::vector<uint32_t>> indices = std::make_unique<std::vector<uint32_t>>(); 
	std::unique_ptr<std::vector<star::Vertex>> vertices = std::make_unique<std::vector<star::Vertex>>();
	 
	std::unique_ptr<float> line = std::unique_ptr<float>(new float[dataset->GetRasterXSize() * dataset->GetRasterYSize()]);

	band->RasterIO(GF_Read, 0, 0, nXSize, nYSize, line.get(), nXSize, nYSize, GDT_Float32, 0, 0);

	float xTexStep = 1.0f / nXSize;
	float yTexStep = 1.0f / nYSize; 

	//calculate locations
	for (int i = 0; i < nYSize; i++) {
		for (int j = 0; j < nXSize; j++) {
			auto location = toECEF(glm::vec3{
				this->upperLeft.x - j * vert / nXSize,
				this->upperLeft.y + i * horz / nYSize,
				line.get()[i * nXSize + j]
			});
		
			terrainCenter = (terrainCenter + location) / glm::vec3(2.0f, 2.0f, 2.0f);
			glm::vec2 texCoord = glm::vec2(j * xTexStep, i * yTexStep);
			vertices->push_back(star::Vertex(location, glm::vec3(), glm::vec3(), texCoord));
		}
	}

	//calculate normals
	{
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
					indices->push_back(center);
					indices->push_back(upperLeft);
					indices->push_back(centerLeft);
					//2
					indices->push_back(center);
					indices->push_back(upperCenter);
					indices->push_back(upperLeft);

					if (i != i - 1 && j == j - 1)
					{
						//side piece
						//cant do 3,4,5,6,
						//7
						indices->push_back(center);
						indices->push_back(lowerLeft);
						indices->push_back(lowerCenter);
						//8
						indices->push_back(center);
						indices->push_back(centerLeft);
						indices->push_back(lowerLeft);

					}
					else if (i == i - 1 && j != j - 1)
					{
						//bottom piece
						//cant do 5,6,7,8
						//3
						indices->push_back(center);
						indices->push_back(upperRight);
						indices->push_back(upperCenter);
						//4
						indices->push_back(center);
						indices->push_back(centerRight);
						indices->push_back(upperRight);
					}
					else if (i != i - 1 && j != j - 1) {
						//3
						indices->push_back(center);
						indices->push_back(upperRight);
						indices->push_back(upperCenter);
						//4
						indices->push_back(center);
						indices->push_back(centerRight);
						indices->push_back(upperRight);
						//5
						indices->push_back(center);
						indices->push_back(lowerRight);
						indices->push_back(centerRight);
						//6
						indices->push_back(center);
						indices->push_back(lowerCenter);
						indices->push_back(lowerRight);
						//7
						indices->push_back(center);
						indices->push_back(lowerLeft);
						indices->push_back(lowerCenter);
						//8
						indices->push_back(center);
						indices->push_back(centerLeft);
						indices->push_back(lowerLeft);
					}

				}
				indexCounter++;
			}
		}
	}

	//calculate normals
	for (int i = 0; i < indices->size(); i += 3) {
		auto& vert1 = vertices->at(indices->at(i));
		auto& vert2 = vertices->at(indices->at(i + 1));
		auto& vert3 = vertices->at(indices->at(i + 2));

		glm::vec3 edge1 = vert2.pos - vert1.pos;
		glm::vec3 edge2 = vert3.pos - vert1.pos;
		glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

		vert1.normal += normal;
		vert2.normal += normal;
		vert3.normal += normal;
	}
	
	//set all verts around origin 
	for (auto& vert : *vertices) {
		vert.pos = vert.pos - terrainCenter;
		vert.normal = glm::normalize(vert.normal); 
	}

	auto texture = std::make_shared<star::Texture>(this->texturePath);
	auto material = std::make_shared<star::TextureMaterial>(texture);
	this->meshes.push_back(std::make_unique<star::StarMesh>(*vertices, *indices, material, false));

	GDALClose(dataset);

	std::unique_ptr<star::StarBuffer> vertStagingBuffer; 
	{
		vertStagingBuffer = std::make_unique<star::StarBuffer>(device,
			sizeof(star::Vertex),
			vertices->size(),
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

		vertStagingBuffer->map(); 
		vertStagingBuffer->writeToBuffer(vertices->data(), vertices->size() * sizeof(star::Vertex));
		vertStagingBuffer->unmap(); 
	}

	std::unique_ptr<star::StarBuffer> indexStagingBuffer; 
	{
		indexStagingBuffer = std::make_unique<star::StarBuffer>(device,
			sizeof(uint32_t),
			indices->size(),
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

		indexStagingBuffer->map(); 
		indexStagingBuffer->writeToBuffer(indices->data(), indices->size() * sizeof(uint32_t));
		indexStagingBuffer->unmap(); 
	}

	return std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>(std::move(vertStagingBuffer), std::move(indexStagingBuffer));
}
