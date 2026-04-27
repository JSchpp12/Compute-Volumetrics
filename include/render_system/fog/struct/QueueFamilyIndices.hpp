#pragma once

namespace render_system::fog
{
struct QueueFamilyIndices
{
    uint32_t graphics{0};
    uint32_t transfer{0};
    uint32_t compute{0};
};
} // namespace render_system::fog