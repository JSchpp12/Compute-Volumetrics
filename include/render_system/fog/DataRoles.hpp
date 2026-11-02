#pragma once

#include <string_view>

namespace render_system::fog::data_roles
{
constexpr std::string_view LightTransmittanceMap = "stLTM";
constexpr std::string_view LightTransmittanceMapSampled = "stLTMS";
constexpr std::string_view LightTransmittanceNoise = "stLTN";
} // namespace render_system::fog::data_roles
