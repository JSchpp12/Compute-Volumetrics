#pragma once

#include "config/AppConfigInfo.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace config
{
enum class LoadStatus
{
    Loaded,
    CreatedDefault,
    ValidationError
};

class AppConfigLoader
{
  public:
    static std::pair<std::unique_ptr<AppConfigInfo>, LoadStatus> LoadFromArgs(int argc, char **argv);
    static void LogConfig(const AppConfigInfo &cfg);

  private:
    static std::unique_ptr<AppConfigInfo> ApplyCliOverrides(int argc, char **argv, std::unique_ptr<AppConfigInfo> cfg);
    static void CreateDefaultFile(const std::filesystem::path &path, const AppConfigInfo &cfg);
    //Checks if the 
    static std::vector<std::string> PatchMissingFields(nlohmann::json &j, const std::filesystem::path &path);
};
} // namespace config