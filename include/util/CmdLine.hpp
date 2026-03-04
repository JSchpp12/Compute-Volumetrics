#pragma once

#include <string>
#include <optional>

namespace util::CmdLine
{
std::optional<std::string> TryGetArgValue(int argc, char **argv, const std::string &arg) noexcept; 

std::string GetTerrainPath(int argc, char **argv);

std::string GetSimControllerFilePath(int argc, char **argv);
}