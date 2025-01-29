#include "SampledVolumeTexture.hpp"

std::unique_ptr<star::StarBuffer> SampledVolumeTexture::loadImageData(star::StarDevice& device)
{
	vk::DeviceSize imageSize = (vk::DeviceSize(this->creationSettings.width) * vk::DeviceSize(this->creationSettings.height) * vk::DeviceSize(this->creationSettings.channels) * vk::DeviceSize(this->creationSettings.depth)) 
		* vk::DeviceSize(4);

	std::unique_ptr<star::StarBuffer> stagingBuffer = std::make_unique<star::StarBuffer>(
		device,
		imageSize,
		uint32_t(1),
		VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::SharingMode::eConcurrent
	);

	stagingBuffer->map(); 
	
	std::vector<float> flattenedData; 
	int floatCounter = 0;
	for (int i = 0; i < this->sampledData->size(); i++) {
		for (int j = 0; j < this->sampledData->at(i).size(); j++) {
			for (int k = 0; k < this->sampledData->at(i).at(j).size(); k++) {
				flattenedData.push_back(this->sampledData->at(i).at(j).at(k));
				floatCounter++; 
			}
		}
	}
	stagingBuffer->writeToBuffer(flattenedData.data(), sizeof(float) * floatCounter); 

	stagingBuffer->unmap(); 

	return stagingBuffer;
}
