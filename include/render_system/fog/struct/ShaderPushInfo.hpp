#pragma once

#include <cstdint>

namespace render_system::fog
{
struct ShaderPushInfo
{
    uint32_t flags{0};
    uint32_t stepsPerDispatch{0};
};
} // namespace render_system::fog