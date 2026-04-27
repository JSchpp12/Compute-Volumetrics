#pragma once

namespace render_system::fog
{
struct alignas(16) RayMarchState
{
    float radiance[3];
    float transparency;
    float direction[3];
    float t0;
    float origin[3];
    float tEnd;
    uint32_t targetPixel[2];
    float rayDistanceTraveled;
    float localDepthDistance;
    uint32_t status; //bool in shader
    uint32_t _pad[3];//explicit padding
};

} // namespace render_system::fog