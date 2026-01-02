#include "RandomValueTexture.hpp"

#include <random>

std::unique_ptr<std::vector<float>> RandomValueTexture::loadTexture(const uint32_t &width, const uint32_t &height) const
{
    std::random_device rDevice; 
    std::mt19937 gen(rDevice()); 
    std::uniform_real_distribution<float> dist(0.0f, 0.5f); 

    auto data = std::make_unique<std::vector<float>>(width * height);

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            std::size_t idx = static_cast<size_t>(y * width + x);
            data->at(idx) = dist(gen); 
        }
    }

    return data;
}
