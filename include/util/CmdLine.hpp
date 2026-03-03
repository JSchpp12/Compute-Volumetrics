#pragma once

#include <string>
#include <iostream>
#include <optional>

namespace util::CmdLine
{
std::optional<std::string> TryGetArgValue(int argc, char **argv, const std::string &arg) noexcept; 
}