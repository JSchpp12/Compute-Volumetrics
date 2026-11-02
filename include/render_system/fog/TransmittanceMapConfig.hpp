#pragma once

#include <array>
#include <cstdint>

namespace render_system::fog
{
/// Resolution (x, y, z) of the precomputed light transmittance map. Shared by
/// the volume's precompute resource setup (Volume::initVolume) and the
/// transmittance cell debug visualization so the two cannot drift apart.
inline constexpr std::array<uint32_t, 3> kTransmittanceMapResolution{1024, 1024, 512};

/// Shadow-map parameters the terrain shadow camera fits its light window with
/// (TerrainShadowRenderPhaseProvider / ShadowCameraController). The transmittance map is
/// carved out of that same window, so anything mirroring the precompute's projection has to
/// use these values. Keep them in sync with the terrain shadow phase construction.
inline constexpr std::array<uint32_t, 2> kShadowMapResolution{2048, 2048};
inline constexpr float kShadowCasterPadding{512.0f};
} // namespace render_system::fog