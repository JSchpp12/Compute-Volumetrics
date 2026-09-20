#pragma once

#include <star_terrain/rendering/ShadowCasterInfo.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <vector>

/// Pure-math planning for the transmittance map debug visualization. Computes
/// where to place one wireframe cube instance per map texel for a fixed-size
/// texel window centered on the main camera. No Vulkan objects are touched
/// here, so the math is unit testable on its own.
namespace transmittance_viz
{
/// World-space placement of one map texel cell: where the instance should sit
/// and how large the (unit) cube mesh should be scaled to match the cell.
struct CellPlacement
{
    glm::vec3 position{};
    glm::vec3 size{};
};

struct WindowPlacements
{
    /// One placement per window cell. Index order matches WindowOffsetAt.
    std::vector<CellPlacement> cells;
    /// World-space full size of one cell. The orthographic light projection
    /// makes the grid uniform, so every cell shares this size.
    glm::vec3 cellWorldSize{};
    /// Texel the window is centered on after clamping to the map bounds.
    std::array<int, 3> centerTexel{};
};

/// Number of cells drawn around the camera along each map axis.
inline constexpr std::array<int, 3> kDefaultWindowSize{10, 10, 10};

/// Total number of cells in a window.
inline size_t WindowCellCount(const std::array<int, 3> &windowSize)
{
    return static_cast<size_t>(windowSize[0]) * static_cast<size_t>(windowSize[1]) * static_cast<size_t>(windowSize[2]);
}

/// Decode a flat window index into its (x, y, z) offset from the window's
/// lowest corner.
std::array<int, 3> WindowOffsetAt(size_t index, const std::array<int, 3> &windowSize);

/// Compute world-space placements for the window of map texels around the
/// camera. The window follows the camera in whole texel steps and is clamped
/// so it always stays fully inside the map.
///
/// The texel <-> NDC mapping mirrors volume_precomputeLightTransmittance.comp:
/// x/y NDC spans [-1, 1] over the map extents and z NDC spans [0, 1] over the
/// light frustum depth (GLM_FORCE_DEPTH_ZERO_TO_ONE is enabled project-wide).
WindowPlacements ComputeWindowPlacements(const star::StarCamera &camera, const glm::vec3 &lightDirection,
                                         const std::array<uint32_t, 3> &mapResolution, const std::array<int, 3> &windowSize);
} // namespace transmittance_viz