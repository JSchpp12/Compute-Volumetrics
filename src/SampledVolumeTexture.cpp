#include "SampledVolumeTexture.hpp"

#include "CastHelpers.hpp"

star::StarTexture::TextureCreateSettings SampledVolumeRequest::getCreateArgs() const{
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

void SampledVolumeRequest::writeData(star::StarBuffer &buffer) const{
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

void SampledVolumeRequest::copyFromTransferSRCToDST(star::StarBuffer& srcBuffer, star::StarTexture& dstTexture, vk::CommandBuffer& commandBuffer) const{
	
	uint32_t width = star::CastHelpers::int_to_unsigned_int(this->sampledData->size());
	uint32_t height = star::CastHelpers::int_to_unsigned_int(this->sampledData->at(0).size());

    vk::BufferImageCopy region{}; 
    region.bufferOffset = 0; 
    region.bufferRowLength = 0; 
    region.bufferImageHeight = 0; 

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor; 
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{}; 
    region.imageExtent = vk::Extent3D{
		width,
		height, 
        1
    };

    commandBuffer.copyBufferToImage(srcBuffer.getVulkanBuffer(), dstTexture.getImage(), vk::ImageLayout::eTransferDstOptimal, region);
}

std::unique_ptr<star::TransferRequest::Texture> SampledVolumeController::createTransferRequest(const vk::PhysicalDevice& physicalDevice){
	return std::make_unique<SampledVolumeRequest>(std::move(this->sampledData)); 
}
