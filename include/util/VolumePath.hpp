#pragma once

#include <starlight/util/io/IOError.hpp>

#include <expected>
#include <filesystem>
#include <string>

namespace star::core
{
class CommandBus;
}

namespace util
{
std::expected<std::filesystem::path, star::io::IOError> ResolveVolumeDirectory(const star::core::CommandBus &bus,
                                                                               const std::string &volumeName);
} // namespace util
