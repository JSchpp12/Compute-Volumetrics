#include "util/CmdLine.hpp"

#include <optional>
#include <logging/LoggingFactory.hpp>

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
    {
        std::cerr << "Terrain dir must be provided with arg '--terrain'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return terrainPath.value();
}

std::string GetSimControllerFilePath(int argc, char **argv)
{
    auto controllerPath = util::CmdLine::TryGetArgValue(argc, argv, "--controller");
    if (!controllerPath.has_value())
    {
        std::cerr << "Simulation controller path file must be provided with arg '--controller'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return controllerPath.value();
}
std::string GetConfigFilePath(int argc, char **argv)
{
    auto file = util::CmdLine::TryGetArgValue(argc, argv, "--config");
    if (!file.has_value())
    {
        file = "StarEngine.cfg";
        star::core::logging::info("No config file provided, using default StarEngine.cfg");
    }

    return file.value();
}
} // namespace util::CmdLine