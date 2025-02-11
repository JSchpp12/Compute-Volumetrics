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
	TerrainChunkProcessor chunkProcessor = TerrainChunkProcessor(chunks.data(), &device);
	oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, chunks.size()), chunkProcessor);
	std::cout << "Done" << std::endl;

	std::vector<std::unique_ptr<star::StarBuffer>> chunkVertStagingBuffers = std::vector<std::unique_ptr<star::StarBuffer>>(chunks.size());
	std::vector<std::unique_ptr<star::StarBuffer>> chunkIndStagingBuffers = std::vector<std::unique_ptr<star::StarBuffer>>(chunks.size());

	//go through chunks and calculate size of all vertex buffers
	vk::DeviceSize totalVertBufferSize = 0;
	vk::DeviceSize totalIndBufferSize = 0;
	for (auto& chunk : chunks){
		this->meshes.emplace_back(chunk.getMesh());
		totalVertBufferSize += sizeof(star::Vertex) * this->meshes.back()->getNumVerts();
		totalIndBufferSize += this->meshes.back()->getNumIndices();
	}

	std::unique_ptr<star::StarBuffer> vertStagingBuffer = std::make_unique<star::StarBuffer>(
		device,
		sizeof(star::Vertex),
		uint32_t(totalVertBufferSize),
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eConcurrent
	);

	std::unique_ptr<star::StarBuffer> indexStagingBuffer = std::make_unique<star::StarBuffer>(
		device,
		sizeof(uint32_t),
		static_cast<uint32_t>(totalIndBufferSize),
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eConcurrent
	);

	assert(chunks.size() == this->meshes.size() && "Mismatch between number of chunks and meshes");

	{
		vk::DeviceSize offset_verts = 0;
		vk::DeviceSize offset_inds = 0;

		for (int i = 0; i < chunks.size(); i++){
			device.copyBuffer(chunks[i].getVertexBuffer().getVulkanBuffer(), vertStagingBuffer->getVulkanBuffer(), this->meshes[i]->getNumVerts() * sizeof(star::Vertex), offset_verts);
			device.copyBuffer(chunks[i].getIndexBuffer().getVulkanBuffer(), indexStagingBuffer->getVulkanBuffer(), this->meshes[i]->getNumIndices() * sizeof(uint32_t), offset_inds);

			offset_verts += this->meshes[i]->getNumVerts() * sizeof(star::Vertex);
			offset_inds += this->meshes[i]->getNumIndices() * sizeof(uint32_t);
		}
	}

	return std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>(std::move(vertStagingBuffer), std::move(indexStagingBuffer));
}
