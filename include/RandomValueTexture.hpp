#pragma once

#include "textures/TransferRequest_TextureData.hpp"

class RandomValueTexture : public star::TransferRequest::TextureData<float, 1>
{
  public:
    RandomValueTexture(uint32_t width, uint32_t height, uint32_t computeQueueFamilyIndex,
                       const vk::PhysicalDeviceProperties &deviceProps)
        : star::TransferRequest::TextureData<float, 1>(width, height, computeQueueFamilyIndex, deviceProps,
                                                       vk::ImageLayout::eGeneral)
    {
    }
    virtual ~RandomValueTexture() = default;

  protected:
    virtual std::unique_ptr<std::vector<float>> loadTexture(const uint32_t &width,
                                                            const uint32_t &height) const override;
};