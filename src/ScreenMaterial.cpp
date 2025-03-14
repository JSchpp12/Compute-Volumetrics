#include "ScreenMaterial.hpp"

void ScreenMaterial::applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
}

void ScreenMaterial::prep(star::StarDevice& device)
{
}

void ScreenMaterial::buildDescriptorSet(star::StarDevice& device, star::StarShaderInfo::Builder& builder, const int& imageInFlightIndex)
{
	builder.startSet(); 
	builder.add(*computeOutputImages->at(imageInFlightIndex), vk::ImageLayout::eShaderReadOnlyOptimal, true);
}

void ScreenMaterial::cleanup(star::StarDevice& device)
{

}

std::vector<std::pair<vk::DescriptorType, const int>> ScreenMaterial::getDescriptorRequests(const int& numFramesInFlight){
	return std::vector<std::pair<vk::DescriptorType, const int>>{
		std::make_pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, this->computeOutputImages->size()) 
	};
}
