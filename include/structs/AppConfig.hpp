#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

struct AppConfig
{
    std::string volumeName;
    std::string terrainDir;
    std::string engineConfigFile;
    std::string simControllerPath;
    std::optional<int> overrideRenderingDevice{std::nullopt};

    // Load from a JSON file path
    static AppConfig load(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error("Could not open config file: " + path);

        nlohmann::json j;
        try
        {
            file >> j;
        }
        catch (const nlohmann::json::parse_error &e)
        {
            throw std::runtime_error("Failed to parse config file: " + std::string(e.what()));
        }

        AppConfig cfg;
        cfg.volumeName = j.value("volume_dir", "");
        cfg.terrainDir = j.value("terrain_dir", "");
        cfg.engineConfigFile = j.value("config_file", "");
        if (j.contains("rendering_device_index"))
            cfg.overrideRenderingDevice = j.value("rendering_device_index", -1);
        return cfg;
    }

    // Optional: validate that required fields aren't empty
    void validate() const
    {
        if (volumeName.empty())
            throw std::runtime_error("Config: 'volume_dir' must not be empty");
        if (terrainDir.empty())
            throw std::runtime_error("Config: 'terrain_dir' must not be empty");
    }
};