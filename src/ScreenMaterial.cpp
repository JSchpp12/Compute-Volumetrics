#include "ScreenMaterial.hpp"

void ScreenMaterial::applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
}

void ScreenMaterial::prep(star::StarDevice& device)
{
}

vk::DescriptorSet ScreenMaterial::buildDescriptorSet(star::StarDevice& device, star::StarDescriptorSetLayout& groupLayout, star::StarDescriptorPool& groupPool, const int& imageInFlightIndex)
{
	auto sets = std::vector<vk::DescriptorSet>();
	auto layoutBuilder = star::StarDescriptorSetLayout::Builder(device);
	auto writer = star::StarDescriptorWriter(device, groupLayout, groupPool);

	auto texInfo = vk::DescriptorImageInfo{
		this->computeOutputImages->at(imageInFlightIndex)->getSampler(),
		this->computeOutputImages->at(imageInFlightIndex)->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal };
	writer.writeImage(0, texInfo);

	vk::DescriptorSet newSet = writer.build();

	return newSet;
}

void ScreenMaterial::cleanup(star::StarDevice& device)
{

}
