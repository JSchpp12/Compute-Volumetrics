#pragma once

#include <memory>
#include <vector>

#include "DescriptorModifier.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarMaterial.hpp"
#include "StarShaderInfo.hpp"
#include "StarTexture.hpp"

class ScreenMaterial : public star::StarMaterial
{
  public:
    ScreenMaterial(std::vector<std::unique_ptr<star::StarTexture>> *computeOutputImages)
        : computeOutputImages(computeOutputImages) {};

  protected:
    std::vector<std::unique_ptr<star::StarTexture>> *computeOutputImages = nullptr;

    void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder &constBuilder) override;
    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int &numFramesInFlight) override;
    void prep(star::StarDevice &device) override;
    void buildDescriptorSet(star::StarDevice &device, star::StarShaderInfo::Builder &builder,
                            const int &imageInFlightIndex) override;
    void cleanup(star::StarDevice &device) override;
};
