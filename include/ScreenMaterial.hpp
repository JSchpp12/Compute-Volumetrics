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
    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(
        const int &numFramesInFlight) const override;

    void addComputeWriteToImage(std::shared_ptr<star::StarTextures::Texture> computeTexture);

    void addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &builder) const override;

  protected:
    std::vector<std::shared_ptr<star::StarTextures::Texture>> m_computeOutputImages =
        std::vector<std::shared_ptr<star::StarTextures::Texture>>();

    std::unique_ptr<star::StarShaderInfo> buildShaderInfo(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight, star::StarShaderInfo::Builder builder) override; 
};
