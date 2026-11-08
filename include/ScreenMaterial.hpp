#pragma once

#include <memory>
#include <vector>

#include "StarMaterial.hpp"
#include "StarShaderInfo.hpp"
#include "StarTextures/Texture.hpp"
#include "starlight/virtual/StarMaterial.hpp"
#include "starlight/wrappers/graphics/StarDescriptorBuilders.hpp"

class ScreenMaterial : public star::StarMaterial
{
  public:
    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(int numFramesInFlight) const override;

    void setComputeWriteToImages(const std::vector<star::Handle> &computeTextures);

    void addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &builder) const override;

  protected:
    std::vector<star::Handle> m_computeOutputImages = std::vector<star::Handle>();

    std::unique_ptr<star::StarShaderInfo> buildShaderInfo(star::core::device::DeviceContext &context,
                                                          uint8_t numFramesInFlight,
                                                          star::StarShaderInfo::Builder builder) override;
};
