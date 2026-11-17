#pragma once

#include "loader/SceneDescription.hpp"

#include <star_skybox/struct/SkyboxInfo.hpp>
#include <starlight/core/device/DeviceContext.hpp>

#include <filesystem>
#include <memory>

namespace star
{
class StarObject;
class StarCamera;
} // namespace star

namespace star::skybox
{
class SkyboxContext;
} // namespace star::skybox

namespace loader
{
SceneDescription DebugSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                  const std::filesystem::path &terrainPath, bool enableTransmittanceMapDebug,
                                  bool aggressiveTerrainTextureLoading, bool forceFlatTerrains,
                                  const star::StarCamera &camera);

SceneDescription ReleaseSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                    const std::filesystem::path &terrainPath, bool enableTransmittanceMapDebug,
                                    bool aggressiveTerrainTextureLoading, bool forceFlatTerrains,
                                    const star::StarCamera &camera);

/// @brief Builds the skybox scene object around one resolved library entry.
std::shared_ptr<star::StarObject> LoadSkybox(star::core::device::DeviceContext &ctx,
                                             star::skybox::SkyboxContext &skyboxContext,
                                             const std::filesystem::path &skyboxTexturePath,
                                             star::skybox::SkyboxInfo info);
} // namespace loader
