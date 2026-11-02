#pragma once

#include <string_view>

namespace data_roles
{
constexpr std::string_view TerrainShadowMap = "stSMap";
/// Non-compare (raw) sampler view of the terrain shadow depth image.
constexpr std::string_view TerrainShadowDepthRaw = "stSDepthRaw";
} // namespace data_roles