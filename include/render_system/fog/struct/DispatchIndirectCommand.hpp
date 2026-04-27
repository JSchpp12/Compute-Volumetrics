#pragma once

namespace render_system::fog
{
struct DispatchIndirectCommand
{
    uint32_t values[3]; 
};

struct ActiveRayHeader
{
    DispatchIndirectCommand cmdValues;
    uint32_t activeCount{0};
};
} // namespace render_system::fog