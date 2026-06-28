#pragma once

#include <nlohmann/json.hpp>

namespace config
{
struct InteractiveConfigInfo;

void to_json(nlohmann::json &j, const InteractiveConfigInfo &v);
void from_json(const nlohmann::json &j, InteractiveConfigInfo &v);
} // namespace config