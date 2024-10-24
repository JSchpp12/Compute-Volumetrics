#pragma once 

#include "StarMaterial.hpp"
#include "Texture.hpp"
#include "StarDescriptorBuilders.hpp"
#include "DescriptorModifier.hpp"
#include "StarShaderInfo.hpp"

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
	void buildDescriptorSet(star::StarDevice& device, star::StarShaderInfo::Builder& builder, const int& imageInFlightIndex) override;
	void cleanup(star::StarDevice& device) override;
};
