#pragma once

#include <optional>
#include <string>

namespace config
{
struct AppConfigInfo;

std::optional<std::string> validateAppConfig(const AppConfigInfo &cfg);
} // namespace config