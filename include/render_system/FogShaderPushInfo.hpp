#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace render_system
{
struct FogShaderPushInfo
{
    glm::uvec2 tileOffset;
};
} // namespace render_system