#pragma once

#include <optional>
#include <string>

namespace util::CmdLine
{
std::optional<std::string> TryGetArgValue(int argc, char **argv, const std::string &arg) noexcept;

std::string GetTerrainPath(int argc, char **argv);

std::string GetSimControllerFilePath(int argc, char **argv);

std::string GetConfigFilePath(int argc, char **argv);

std::string GetVolumeDirPath(int argc, char **argv);

std::optional<int> TryGetDeviceIndexOverride(int argc, char **argv);
} // namespace util::CmdLine