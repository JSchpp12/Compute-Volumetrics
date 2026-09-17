#pragma once

#include <array>

namespace render_system::fog
{
/// Resolution (x, y, z) of the precomputed light transmittance map. Shared by
/// the volume's precompute resource setup (Volume::initVolume) and the
/// transmittance cell debug visualization so the two cannot drift apart.
inline constexpr std::array<int, 3> kTransmittanceMapResolution{1024, 1024, 512};
} // namespace render_system::fog