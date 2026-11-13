#pragma once

#include "loader/SceneDescription.hpp"

#include <star_skybox/struct/SkyboxInfo.hpp>
#include <starlight/core/device/DeviceContext.hpp>

#include <filesystem>
#include <memory>

namespace star
{
class StarObject;
}

namespace star::skybox
{
class SkyboxContext;
}

namespace loader
{
SceneDescription DebugSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                  const std::filesystem::path &terrainPath, bool enableTransmittanceMapDebug,
                                  bool aggressiveTerrainTextureLoading, bool forceFlatTerrains);

SceneDescription ReleaseSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                    const std::filesystem::path &terrainPath, bool enableTransmittanceMapDebug,
                                    bool aggressiveTerrainTextureLoading, bool forceFlatTerrains);

/// @brief Builds the skybox scene object around one resolved library entry.
/// @details The single object-side creator for the skybox: resolves nothing itself (the caller supplies the entry's
/// description), makes the resource through MakeSkybox, and wires the mesh, shaders, material provider, and render
/// overrides. The SkyboxController calls this for the initial object and swaps the resulting material's resource for
/// later selections.
std::shared_ptr<star::StarObject> LoadSkybox(star::core::device::DeviceContext &ctx,
                                             star::skybox::SkyboxContext &skyboxContext,
                                             const std::filesystem::path &skyboxTexturePath,
                                             star::skybox::SkyboxInfo info);
} // namespace loader
