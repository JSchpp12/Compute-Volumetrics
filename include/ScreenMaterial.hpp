#pragma once 

#include "StarMaterial.hpp"
#include "StarImage.hpp"
#include "StarDescriptorBuilders.hpp"
#include "DescriptorModifier.hpp"
#include "StarShaderInfo.hpp"

#include <vector>
#include <memory>

class ScreenMaterial : public star::StarMaterial {

public: 
	ScreenMaterial(std::vector<std::unique_ptr<star::StarImage>>* computeOutputImages)
		: computeOutputImages(computeOutputImages) {};

protected:
	std::vector<std::unique_ptr<star::StarImage>>* computeOutputImages = nullptr;

	void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
	std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;
 	void prep(star::StarDevice& device) override;
	void buildDescriptorSet(star::StarDevice& device, star::StarShaderInfo::Builder& builder, const int& imageInFlightIndex) override;
	void cleanup(star::StarDevice& device) override;
};
