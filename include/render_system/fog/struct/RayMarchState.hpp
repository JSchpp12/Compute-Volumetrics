#pragma once

namespace render_system::fog
{
struct alignas(16) RayMarchState
{
    float radiance[3];
    float _pad0;
    float transparency;
    float t0;
    float tEnd;
    float rayDistanceTraveled;
    uint32_t status; //bool in shader
    uint32_t _pad1;
};

} // namespace render_system::fog