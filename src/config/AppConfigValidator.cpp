#include "config/AppConfigValidator.hpp"
#include "config/AppConfigInfo.hpp"

#include <starlight/util/io/SearchPolicy.hpp>

#include <filesystem>
#include <string>

namespace config
{
std::optional<std::string> validateAppConfig(const AppConfigInfo &cfg)
{
    namespace io = star::io;
    using star::io::PathKind;

    if (cfg.volumeName.empty())
        return "Config: 'volumeName' must not be empty";
    if (cfg.terrainDir.empty())
        return "Config: 'terrainDir' must not be empty";
    if (cfg.skyboxPath.empty())
        return "Config: 'skyboxPath' must not be empty";

    {
        auto resolved = io::resolvePath(std::filesystem::path{cfg.terrainDir}, PathKind::directory);
        if (!resolved)
            return "Config: 'terrainDir' does not exist or is not a directory: " + resolved.error().toString();
    }

    if (!cfg.simControllerPath.empty())
    {
        auto resolved = io::resolvePath(std::filesystem::path{cfg.simControllerPath}, PathKind::file);
        if (!resolved)
            return "Config: 'simControllerPath' does not exist or is not a regular file: " +
                   resolved.error().toString();
    }
    return std::nullopt;
}
} // namespace config