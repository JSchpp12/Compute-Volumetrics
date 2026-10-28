#pragma once

#include <string_view>

/// Central table of pipeline variant names. Variants are selected by name, so every family's names live here instead
/// of being spelled out at each call site.
namespace render_system::fog::variant_names
{
/// Transmittance precompute variants.
namespace transmittance
{
inline constexpr std::string_view Low = "low";
inline constexpr std::string_view High = "high";
} // namespace transmittance
} // namespace render_system::fog::variant_names
