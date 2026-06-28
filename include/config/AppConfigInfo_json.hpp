#pragma once

#include <nlohmann/json.hpp>

namespace config
{
struct AppConfigInfo;

void to_json(nlohmann::json &j, const AppConfigInfo &v);
void from_json(const nlohmann::json &j, AppConfigInfo &v);
} // namespace config