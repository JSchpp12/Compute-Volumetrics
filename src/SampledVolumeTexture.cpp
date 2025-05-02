#include "SampledVolumeTexture.hpp"

star::StarTexture::TextureCreateSettings SampledVolumeRequest::getCreateArgs(const vk::PhysicalDeviceProperties &deviceProperties) const
{
	return star::StarTexture::TextureCreateSettings{
		static_cast<int>(sampledData->size()),
		static_cast<int>(sampledData->at(0).size()),
		1,
		1,
		4,
		vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::Format::eR32Sfloat,
		{},
		vk::ImageAspectFlagBits::eColor,
		VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
		VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		false, true, {}, 1.0f, vk::Filter::eNearest, "SampledVolume"};
}

void SampledVolumeRequest::writeData(star::StarBuffer &buffer) const
{
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

	buffer.map();
	
	buffer.writeToBuffer(flattenedData.data(), sizeof(float) * floatCounter); 

	buffer.unmap();
}

std::unique_ptr<star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>> SampledVolumeController::createTransferRequest()
{
    return std::make_unique<SampledVolumeRequest>(std::move(this->sampledData));
}
