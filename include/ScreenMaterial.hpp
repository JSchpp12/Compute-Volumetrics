#pragma once

#include <memory>
#include <vector>

#include "DescriptorModifier.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarMaterial.hpp"
#include "StarShaderInfo.hpp"
#include "StarTextures/Texture.hpp"

class ScreenMaterial : public star::StarMaterial
{
  public:
    ScreenMaterial(std::vector<std::unique_ptr<star::StarTextures::Texture>> &computeOutputImages)
        : computeOutputImages(computeOutputImages) {};

  protected:
    std::vector<std::unique_ptr<star::StarTextures::Texture>> &computeOutputImages;

    void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder &constBuilder) override;
    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int &numFramesInFlight) override;
    void prep(star::core::device::DeviceContext &device) override;
    void buildDescriptorSet(star::core::device::DeviceContext &device, star::StarShaderInfo::Builder &builder,
                            const int &imageInFlightIndex) override;
    void cleanup(star::core::device::DeviceContext &device) override;
};
