#pragma once

#include <glm/glm.hpp>

namespace render_system::fog
{
struct DispatchInfo
{
    vk::Buffer indirectBuffer{VK_NULL_HANDLE};
};
} // namespace render_system