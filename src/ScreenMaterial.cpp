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
	builder.add(*computeOutputImages->at(imageInFlightIndex), vk::ImageLayout::eShaderReadOnlyOptimal);
}

void ScreenMaterial::cleanup(star::StarDevice& device)
{

}
