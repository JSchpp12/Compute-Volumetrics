#include "util/Color.hpp"

#include <cmath>

namespace util
{
glm::vec3 HSVToRGB(float h, float s, float v)
{
    h = std::fmod(h, 1.0f);
    if (h < 0.0f)
        h += 1.0f;

    float c = v * s;
    float x = c * (1.0f - std::fabs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    if (h < 1.0f / 6.0f)
    {
        r = c;
        g = x;
        b = 0.0f;
    }
    else if (h < 2.0f / 6.0f)
    {
        r = x;
        g = c;
        b = 0.0f;
    }
    else if (h < 3.0f / 6.0f)
    {
        r = 0.0f;
        g = c;
        b = x;
    }
    else if (h < 4.0f / 6.0f)
    {
        r = 0.0f;
        g = x;
        b = c;
    }
    else if (h < 5.0f / 6.0f)
    {
        r = x;
        g = 0.0f;
        b = c;
    }
    else
    {
        r = c;
        g = 0.0f;
        b = x;
    }

    return glm::vec3(r + m, g + m, b + m);
}
} // namespace util