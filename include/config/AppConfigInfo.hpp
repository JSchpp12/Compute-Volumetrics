#pragma once

#include "config/InteractiveConfigInfo.hpp"

#include <optional>
#include <string>

namespace config
{
struct AppConfigInfo
{
    std::string volumeName;
    std::string terrainDir;
    std::string simControllerPath;
    std::string engineConfigPath;
    InteractiveConfigInfo interactiveConfig{};
    std::optional<int> overrideRenderingDevice{std::nullopt};
    bool enableDistanceMarkers{false};
    bool enableCutoffHighlighting{false};
};
} // namespace config