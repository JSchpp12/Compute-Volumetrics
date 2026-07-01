#include "config/AppConfigLoader.hpp"
#include "config/AppConfigValidator.hpp"
#include "config/AppConfigInfo_json.hpp"
#include "util/CmdLine.hpp"

#include <starlight/common/ConfigFile.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace config
{
std::pair<std::unique_ptr<AppConfigInfo>, LoadStatus> AppConfigLoader::LoadFromArgs(int argc, char **argv)
{
    auto cfg = std::make_unique<AppConfigInfo>();

    auto appConfigPath = util::CmdLine::TryGetAppConfigFilePath(argc, argv);
    if (appConfigPath.has_value())
    {
        const std::filesystem::path path{*appConfigPath};
        if (std::filesystem::exists(path))
        {
            std::ifstream file(path);
            nlohmann::json j;
            file >> j;
            file.close();

            if (auto missing = PatchMissingFields(j, path); !missing.empty())
            {
                std::ostringstream oss;
                oss << "AppConfig file at " << path.string()
                    << " is missing required field(s); wrote them with default values: ";
                for (std::size_t i = 0; i < missing.size(); ++i)
                {
                    if (i)
                        oss << ", ";
                    oss << missing[i];
                }
                oss << ". Edit the file and re-run.";
                STAR_THROW(oss.str());
            }

            from_json(j, *cfg);
        }
        else
        {
            CreateDefaultFile(path, *cfg);
            std::ostringstream oss;
            oss << "AppConfig file not found at " << path.string()
                << "; created one with default values. Edit it and re-run.";
            star::core::logging::error(oss.str());
            return {std::move(cfg), LoadStatus::CreatedDefault};
        }
    }

    cfg = ApplyCliOverrides(argc, argv, std::move(cfg));

    try
    {
        if (cfg->engineConfigPath.empty())
            STAR_THROW("Engine config file path must be provided via --config or in the appConfig file");
        star::ConfigFile::load(cfg->engineConfigPath);
    }
    catch (const std::exception &ex)
    {
        star::core::logging::error(std::string{"Failed to load config file for engine: "} + ex.what());
        return {std::move(cfg), LoadStatus::ValidationError};
    }

    if (auto err = validateAppConfig(*cfg))
    {
        star::core::logging::error(*err);
        return {std::move(cfg), LoadStatus::ValidationError};
    }

    return {std::move(cfg), LoadStatus::Loaded};
}

void AppConfigLoader::LogConfig(const AppConfigInfo &cfg)
{
    std::ostringstream oss;
    oss << "AppConfig loaded:\n"
        << "  volumeName:              " << cfg.volumeName << "\n"
        << "  terrainDir:              " << cfg.terrainDir << "\n"
        << "  simControllerPath:       " << cfg.simControllerPath << "\n"
        << "  engineConfigPath:        " << cfg.engineConfigPath << "\n"
        << "  overrideRenderingDevice: "
        << (cfg.overrideRenderingDevice.has_value() ? std::to_string(*cfg.overrideRenderingDevice)
                                                    : std::string("<none>"))
        << "\n"
        << "  enableDistanceMarkers:   " << (cfg.enableDistanceMarkers ? "true" : "false") << "\n"
        << "  enableCutoffHighlighting: " << (cfg.enableCutoffHighlighting ? "true" : "false") << "\n"
        << "  interactiveConfig:\n"
        << "    cameraMovementSpeed:   " << cfg.interactiveConfig.cameraMovementSpeed << "\n"
        << "    cameraSensitivity:     " << cfg.interactiveConfig.cameraSensitivity << "\n"
        << "    objectMovementSpeed:   " << cfg.interactiveConfig.objectMovementSpeed;
    star::core::logging::info(oss.str());
}

std::unique_ptr<AppConfigInfo> AppConfigLoader::ApplyCliOverrides(int argc, char **argv,
                                                                      std::unique_ptr<AppConfigInfo> cfg)
{
    if (auto v = util::CmdLine::TryGetArgValue(argc, argv, "--volume"))
        cfg->volumeName = std::move(*v);
    if (auto v = util::CmdLine::TryGetArgValue(argc, argv, "--terrain"))
        cfg->terrainDir = std::move(*v);
    if (auto v = util::CmdLine::TryGetArgValue(argc, argv, "--controller"))
        cfg->simControllerPath = std::move(*v);
    if (auto v = util::CmdLine::TryGetArgValue(argc, argv, "--config"))
        cfg->engineConfigPath = std::move(*v);
    if (auto v = util::CmdLine::TryGetDeviceIndexOverride(argc, argv))
        cfg->overrideRenderingDevice = *v;
    if (util::CmdLine::DoesContainOption(argc, argv, "--enableDistanceMarkers"))
        cfg->enableDistanceMarkers = true;
    if (util::CmdLine::DoesContainOption(argc, argv, "--enableCutoffHighlighting"))
        cfg->enableCutoffHighlighting = true;

    return cfg;
}

void AppConfigLoader::CreateDefaultFile(const std::filesystem::path &path, const AppConfigInfo &cfg)
{
    nlohmann::json j;
    to_json(j, cfg);
    std::ofstream out(path);
    out << j.dump(4);
    out.close();
}

std::vector<std::string> AppConfigLoader::PatchMissingFields(nlohmann::json &j, const std::filesystem::path &path)
{
    AppConfigInfo defaults{};
    nlohmann::json dj;
    to_json(dj, defaults);

    std::vector<std::string> added;

    for (auto it = dj.begin(); it != dj.end(); ++it)
    {
        const std::string key = it.key();

        if (!j.contains(key))
        {
            j[key] = it.value();
            added.push_back(key);
        }
        else if (it.value().is_object() && j[key].is_object())
        {
            auto &nested = j[key];
            const auto &nestedDefaults = it.value();
            for (auto nit = nestedDefaults.begin(); nit != nestedDefaults.end(); ++nit)
            {
                if (!nested.contains(nit.key()))
                {
                    nested[nit.key()] = nit.value();
                    added.push_back(key + "." + nit.key());
                }
            }
        }
    }

    if (!added.empty())
    {
        std::ofstream out(path);
        out << j.dump(4);
        out.close();
    }

    return added;
}
} // namespace config