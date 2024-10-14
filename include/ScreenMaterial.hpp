#pragma once 

#include "StarMaterial.hpp"
#include "Texture.hpp"
#include "StarDescriptors.hpp"
#include "DescriptorModifier.hpp"

#include <vector>
#include <memory>

class ScreenMaterial : public star::StarMaterial {

public: 
	ScreenMaterial(std::vector<std::unique_ptr<star::Texture>>* computeOutputImages)
		: computeOutputImages(computeOutputImages) {};

protected:
	std::vector<std::unique_ptr<star::Texture>>* computeOutputImages = nullptr;

	void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
	void prep(star::StarDevice& device) override;
	vk::DescriptorSet buildDescriptorSet(star::StarDevice& device, star::StarDescriptorSetLayout& groupLayout, star::StarDescriptorPool& groupPool, const int& imageInFlightIndex) override;
	void cleanup(star::StarDevice& device) override;
};
