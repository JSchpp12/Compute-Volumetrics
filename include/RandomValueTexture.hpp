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

  protected:
    float generateRandomValue(const float &floor, const float &ceil) const
    {
        return 0.0f;
    }

    virtual std::unique_ptr<std::vector<float>> loadTexture(const uint32_t &width,
                                                            const uint32_t &height) const override
    {
        auto data = std::make_unique<std::vector<float>>(width * height);

        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                std::size_t idx = static_cast<std::size_t>(y) * width + x;
                data->at(idx) = generateRandomValue(0.0f, 1.0f);
            }
        }

        return data;
    }
};