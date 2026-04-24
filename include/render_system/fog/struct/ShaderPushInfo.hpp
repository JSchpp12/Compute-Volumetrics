#pragma once

namespace render_system::fog
{
struct ShaderPushInfo
{
    uint32_t dispatchOffsetPixels[2];
    uint32_t stepsPerDispatch;
};  
} // namespace render_system::fog