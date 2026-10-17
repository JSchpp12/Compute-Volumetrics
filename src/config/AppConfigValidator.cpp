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

    // Note: the volume *directory* is deliberately not resolved here. It is
    // resolved through the GetPath command (util::ResolveVolumeDirectory), which
    // needs the CommandBus -- born inside the engine, long after this runs. The
    // call that actually consumes the resolved directory, Application::loadScene,
    // reports the same failure with the same IOError. See util/VolumePath.hpp.
    return std::nullopt;
}
} // namespace config