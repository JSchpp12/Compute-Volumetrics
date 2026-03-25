#include "RandomValueTexture.hpp"

#include <random>

std::unique_ptr<std::vector<float>> RandomValueTexture::loadTexture(const uint32_t &width, const uint32_t &height) const
{
    std::random_device rDevice;
    std::mt19937 gen(rDevice());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    auto data = std::vector<float>(width * height * 4);

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            for (uint32_t z = 0; z < 4; z++)
            {
                std::size_t idx = static_cast<size_t>(((y * width + x) * 4) + z);
                data[idx] = dist(gen);
            }
        }
    }

    return std::make_unique<std::vector<float>>(std::move(data));
}
