#include "config/AppConfigValidator.hpp"
#include "config/AppConfigInfo.hpp"

#include <starlight/common/ConfigFile.hpp>

#include <filesystem>

namespace config
{
std::optional<std::string> validateAppConfig(const AppConfigInfo &cfg)
{
    if (cfg.volumeName.empty())
        return "Config: 'volumeName' must not be empty";
    if (cfg.terrainDir.empty())
        return "Config: 'terrainDir' must not be empty";

    const std::filesystem::path terrainDirPath{cfg.terrainDir};
    if (!std::filesystem::is_directory(terrainDirPath))
        return "Config: 'terrainDir' does not exist or is not a directory: " + cfg.terrainDir;

    if (!cfg.simControllerPath.empty())
    {
        const std::filesystem::path simControllerPath{cfg.simControllerPath};
        if (!std::filesystem::is_regular_file(simControllerPath))
            return "Config: 'simControllerPath' does not exist or is not a regular file: " + cfg.simControllerPath;
    }

    const auto mediaDir = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    const std::filesystem::path volumePath = std::filesystem::path(mediaDir) / "volumes" / cfg.volumeName;
    if (!std::filesystem::is_directory(volumePath))
        return "Config: resolved volume path does not exist or is not a directory: " + volumePath.string();

    return std::nullopt;
}
} // namespace config