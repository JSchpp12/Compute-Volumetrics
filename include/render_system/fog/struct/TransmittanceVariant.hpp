#pragma once

#include <cstdint>
#include <string_view>

namespace render_system::fog
{
/// @brief One transmittance precompute pipeline variant: the name that identifies and selects it, and the number of
/// samples it marches per column.
struct TransmittanceVariant
{
    std::string_view name;
    uint32_t samples;
};
} // namespace render_system::fog
