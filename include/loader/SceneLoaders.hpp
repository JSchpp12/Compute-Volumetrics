#pragma once

#include "loader/SceneDescription.hpp"

#include <starlight/core/device/DeviceContext.hpp>

#include <filesystem>

namespace star::skybox
{
class SkyboxContext;
}

namespace loader
{
SceneDescription DebugSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                  const std::filesystem::path &terrainPath, bool enableTransmittanceMapDebug,
                                  bool aggressiveTerrainTextureLoading, bool forceFlatTerrains,
                                  star::skybox::SkyboxContext &skyboxContext);

SceneDescription ReleaseSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                    const std::filesystem::path &terrainPath, bool enableTransmittanceMapDebug,
                                    bool aggressiveTerrainTextureLoading, bool forceFlatTerrains,
                                    star::skybox::SkyboxContext &skyboxContext);
} // namespace loader
