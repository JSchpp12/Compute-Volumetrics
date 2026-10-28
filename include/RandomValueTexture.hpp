#pragma once

#include "textures/TransferRequest_TextureData.hpp"

#include <cstdint>
#include <random>

template <uint32_t TNumChannels>
class RandomValueTexture : public star::TransferRequest::TextureData<float, TNumChannels>
{
  public:
    RandomValueTexture(uint32_t width, uint32_t height, uint32_t computeQueueFamilyIndex,
                       const vk::PhysicalDeviceProperties &deviceProps)
        : star::TransferRequest::TextureData<float, TNumChannels>(width, height, computeQueueFamilyIndex, deviceProps,
                                                                  vk::ImageLayout::eGeneral)
    {
    }
    virtual ~RandomValueTexture() = default;

  protected:
    virtual std::unique_ptr<std::vector<float>> loadTexture(const uint32_t &width,
                                                            const uint32_t &height) const override
    {
        std::random_device rDevice;
        std::mt19937 gen(rDevice());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        auto data = std::vector<float>(width * height * TNumChannels);
        for (auto &value : data)
        {
            value = dist(gen);
        }

        return std::make_unique<std::vector<float>>(std::move(data));
    }
};
