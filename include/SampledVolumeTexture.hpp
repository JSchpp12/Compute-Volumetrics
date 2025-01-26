#pragma once 

#include "StarTexture.hpp"

#include <vector>
#include <memory>

class SampledVolumeTexture : public star::StarTexture {
public:
	SampledVolumeTexture(std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData)
		: sampledData(std::move(sampledData)),
		star::StarTexture(
			star::StarTexture::TextureCreateSettings{
				static_cast<int>(sampledData->size()),
				static_cast<int>(sampledData->at(0).size()),
				1,
				1,
				4,
				vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
				vk::Format::eR32Sfloat,
				vk::ImageAspectFlagBits::eColor,
				VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
				VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
				vk::ImageLayout::eShaderReadOnlyOptimal,
				false, true})
	{
	};

protected:
	std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData = std::unique_ptr<std::vector<std::vector<std::vector<float>>>>(); 

	std::unique_ptr<star::StarBuffer> loadImageData(star::StarDevice& device) override; 
};