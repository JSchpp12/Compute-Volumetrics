#include "config/AppConfigLoader.hpp"
#include "config/AppConfigInfo_json.hpp"
#include "config/AppConfigValidator.hpp"
#include "util/CmdLine.hpp"

#include <starlight/common/ConfigFile.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/util/io/SearchPolicy.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace config
{
/// File name probed when no --appConfig path is given on the command line.
static constexpr const char *DefaultAppConfigFileName = "appConfig.json";

static void ResolveFieldIfPresent(std::string &field, star::io::PathKind kind)
{
    if (field.empty())
        return;

    namespace io = star::io;

    auto resolved = io::resolvePath(std::filesystem::path{field}, kind);
    if (resolved)
    {
        field = resolved.value().string();
    }
}

std::pair<std::unique_ptr<AppConfigInfo>, LoadStatus> AppConfigLoader::LoadFromArgs(int argc, char **argv)
{
    auto cfg = std::make_unique<AppConfigInfo>();

    if (auto appConfigPath = util::CmdLine::TryGetAppConfigFilePath(argc, argv))
    {
        const std::filesystem::path providedPath{*appConfigPath};

        // Resolve the appConfig path before the existence check: probe as-is,
        // then relative to the media directory, then relative to the
        // executable directory. On failure keep the original candidate and let
        // the default-file flow below handle it.
        auto resolvedAppConfig = star::io::resolvePath(providedPath, star::io::PathKind::file);
        const std::filesystem::path path = resolvedAppConfig.value_or(providedPath);

        if (std::filesystem::exists(path))
        {
            if (!LoadFromDisk(path, *cfg))
                return {std::move(cfg), LoadStatus::ValidationError};
        }
        else
        {
            CreateDefaultFile(providedPath, *cfg);
            std::ostringstream oss;
            oss << "AppConfig file not found at " << providedPath.string()
                << "; created one with default values. Edit it and re-run.";
            star::core::logging::error(oss.str());
            return {std::move(cfg), LoadStatus::CreatedDefault};
        }
    }
    else
    {
        // No --appConfig given: fall back to a default appConfig.json
        if (auto resolvedMedia =
                star::io::resolvePath(std::filesystem::path{DefaultAppConfigFileName}, star::io::PathKind::file))
        {
            std::ostringstream oss;
            oss << "No --appConfig provided; loading default config found at " << resolvedMedia.value().string();
            star::core::logging::info(oss.str());

            if (!LoadFromDisk(resolvedMedia.value(), *cfg))
                return {std::move(cfg), LoadStatus::ValidationError};
        }
        else
        {
            star::core::logging::info("No --appConfig provided and no default appConfig.json found; "
                                      "using built-in defaults with any CLI overrides");
        }
    }

    cfg = ApplyCliOverrides(argc, argv, std::move(cfg));

    // Resolve every config-provided path before the engine config is loaded
    // and validated: probe as-is -> media-relative -> exe-relative; winners
    // are rewritten into the fields (absolute, canonical). The media dir is
    // exe-relative now, so no engine-config dependency exists at this point.
    ResolveFieldIfPresent(cfg->engineConfigPath, star::io::PathKind::file);
    ResolveFieldIfPresent(cfg->terrainDir, star::io::PathKind::directory);
    ResolveFieldIfPresent(cfg->simControllerPath, star::io::PathKind::file);

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

bool AppConfigLoader::LoadFromDisk(const std::filesystem::path &path, AppConfigInfo &cfg)
{
    try
    {
        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("failed to open the file");

        nlohmann::json j;
        file >> j;
        file.close();

        // Patch missing fields with defaults, loudly, before validating.
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
            throw std::runtime_error(oss.str());
        }

        from_json(j, cfg);
        return true;
    }
    catch (const std::exception &ex)
    {
        std::ostringstream oss;
        oss << "Failed to load app config file " << path.string() << ": " << ex.what();
        star::core::logging::error(oss.str());
        return false;
    }
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
        << "  enableTransmittanceMapDebug: " << (cfg.enableTransmittanceMapDebug ? "true" : "false") << "\n"
        << "  aggressiveTerrainTextureLoading: " << (cfg.aggressiveTerrainTextureLoading ? "true" : "false") << "\n"
        << "  forceFlatTerrains:       " << (cfg.forceFlatTerrains ? "true" : "false") << "\n"
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
    if (util::CmdLine::DoesContainOption(argc, argv, "--enableTransmittanceMapDebug"))
        cfg->enableTransmittanceMapDebug = true;
    if (util::CmdLine::DoesContainOption(argc, argv, "--aggressiveTerrainTextureLoading"))
        cfg->aggressiveTerrainTextureLoading = true;
    if (util::CmdLine::DoesContainOption(argc, argv, "--forceFlatTerrains"))
        cfg->forceFlatTerrains = true;

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