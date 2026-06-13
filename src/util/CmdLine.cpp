#include "util/CmdLine.hpp"

#include <logging/LoggingFactory.hpp>
#include <starlight/core/Exceptions.hpp>

#include <optional>

namespace util::CmdLine
{
std::optional<std::string> TryGetArgValue(int argc, char **argv, const std::string &arg) noexcept
{
    std::optional<std::string> parsed = std::nullopt;

    for (int i = 0; i < argc; i++)
    {
        std::string cArg = argv[i];

        if (cArg == arg && i + 1 < argc)
        {
            parsed = std::string(argv[++i]);
            break;
        }
    }

    return parsed;
}

std::string GetTerrainPath(int argc, char **argv)
{
    auto terrainPath = util::CmdLine::TryGetArgValue(argc, argv, "--terrain");
    if (!terrainPath.has_value())
        STAR_THROW("Terrain dir must be provided with arg '--terrain'");

    return terrainPath.value();
}

std::string GetSimControllerFilePath(int argc, char **argv)
{
    auto controllerPath = util::CmdLine::TryGetArgValue(argc, argv, "--controller");
    if (!controllerPath.has_value())
        STAR_THROW("Simulation controller path file must be provided with arg '--controller'");

    return controllerPath.value();
}

std::string GetConfigFilePath(int argc, char **argv)
{
    auto file = util::CmdLine::TryGetArgValue(argc, argv, "--config");
    if (!file.has_value())
        STAR_THROW("Config file path was not provided as cmd argument");

    return file.value();
}

std::string GetVolumeDirPath(int argc, char **argv)
{
    auto dir = util::CmdLine::TryGetArgValue(argc, argv, "--volume");
    if (!dir.has_value())
        STAR_THROW("Volume directory was not provided as cmd argument");

    return dir.value();
}

std::optional<int> TryGetDeviceIndexOverride(int argc, char **argv)
{
    auto value = util::CmdLine::TryGetArgValue(argc, argv, "--forceRenderingDeviceIndex");
    if (!value.has_value())
        return std::nullopt;

    return std::stoi(value.value());
}

bool DoesContainOption(int argc, char **argv, const std::string &flag)
{
    for (int i{0}; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == flag)
        {
            return true;
        }
    }
    return false;
}
} // namespace util::CmdLine