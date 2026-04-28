#pragma once

#include <glm/glm.hpp>

namespace render_system::fog
{
struct DispatchInfo
{
    vk::Buffer indirectBuffer{VK_NULL_HANDLE};
    uint32_t shaderOptionFlags{0};
};
} // namespace render_system