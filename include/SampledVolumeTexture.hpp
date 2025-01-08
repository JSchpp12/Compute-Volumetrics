#pragma once 

#include "StarTexture.hpp"

#include <vector>
#include <memory>

class SampledVolumeTexture : public star::StarTexture {
public:
	SampledVolumeTexture(std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData) 
		: sampledData(std::move(sampledData)), 
		star::StarTexture(star::StarTexture::TextureCreateSettings(
			vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst, 
			vk::Format::eR32Sfloat,
			VMA_MEMORY_USAGE_GPU_ONLY,
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 
			false, false, vk::ImageLayout::eGeneral
			)), width(static_cast<int>(sampledData->size())), 
		height(static_cast<int>(sampledData->at(0).size())), 
		depth(static_cast<int>(sampledData->at(0).at(0).size())) {};

protected:
	int width, height, depth; 

	std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData = std::unique_ptr<std::vector<std::vector<std::vector<float>>>>(); 

	// Inherited via StarTexture
	std::optional<std::unique_ptr<unsigned char>> data() override;

	int getWidth() override;

	int getHeight() override;

	int getChannels() override;

	int getDepth() override; 

};