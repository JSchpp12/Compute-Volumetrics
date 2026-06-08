#pragma once

#include "loader/SceneDescription.hpp"

#include <starlight/core/device/DeviceContext.hpp>

#include <filesystem>

namespace loader
{
SceneDescription DebugSceneLoader(star::core::device::DeviceContext &ctx,
                                                                const std::filesystem::path &mediaDirPath,
                                                                const std::filesystem::path &terrainPath);

SceneDescription ReleaseSceneLoader(star::core::device::DeviceContext &ctx,
                                                                  const std::filesystem::path &mediaDirPath,
                                                                  const std::filesystem::path &terrainPath);
} // namespace loader
