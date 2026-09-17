#include "TransmittanceCellPlanner.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace transmittance_viz
{
std::array<int, 3> WindowOffsetAt(const size_t index, const std::array<int, 3> &windowSize)
{
    assert(index < WindowCellCount(windowSize) && "Window index is out of range");

    const size_t planeSize = static_cast<size_t>(windowSize[0]) * static_cast<size_t>(windowSize[1]);
    const size_t planeIndex = index % planeSize;

    return {
        static_cast<int>(planeIndex % static_cast<size_t>(windowSize[0])),
        static_cast<int>(planeIndex / static_cast<size_t>(windowSize[0])),
        static_cast<int>(index / planeSize),
    };
}

WindowPlacements ComputeWindowPlacements(const star::StarCamera &camera, const glm::vec3 &lightDirection,
                                         const std::array<int, 3> &mapResolution, const std::array<int, 3> &windowSize)
{
    for (int axis = 0; axis < 3; ++axis)
    {
        assert(windowSize[axis] > 0 && "Window size must be positive");
        assert(windowSize[axis] <= mapResolution[axis] && "Window cannot be larger than the map");
    }

    const std::array<uint32_t, 2> shadowRes{2048, 2048}; 


    // Same math the terrain shadow camera runs (ShadowCameraTransfer builds its
    // GPU buffer from this projection), recomputed on the CPU for the viz.
    const star::terrain::rendering::ShadowCasterInfo shadowCaster{camera, lightDirection};

    const glm::mat4 worldToLightViewProj =
        shadowCaster.getShadowLightProjectionWithTexelSnapping(shadowRes);
    const glm::mat4 lightViewProjToWorld = glm::inverse(worldToLightViewProj);

    // Where the camera sits in the map's texel space.
    const glm::vec4 cameraClip = worldToLightViewProj * glm::vec4{camera.getPosition(), 1.0f};
    const glm::vec3 cameraNdc{cameraClip.x / cameraClip.w, cameraClip.y / cameraClip.w, cameraClip.z / cameraClip.w};
    const glm::vec3 cameraTexel{
        (cameraNdc.x * 0.5f + 0.5f) * static_cast<float>(mapResolution[0]),
        (cameraNdc.y * 0.5f + 0.5f) * static_cast<float>(mapResolution[1]),
        cameraNdc.z * static_cast<float>(mapResolution[2]),
    };

    // Clamp the window center so the whole window stays inside the map. For
    // even window sizes the window extends one cell further below the center
    // than above it.
    std::array<int, 3> center{};
    for (int axis = 0; axis < 3; ++axis)
    {
        const int half = windowSize[axis] / 2;
        const int lowestCenter = half;
        const int highestCenter = mapResolution[axis] - windowSize[axis] + half;

        center[axis] = std::clamp(static_cast<int>(std::floor(cameraTexel[axis])), lowestCenter,
                                  std::max(lowestCenter, highestCenter));
    }

    // The orthographic projection makes NDC -> world affine, so the linear part
    // of the inverse maps NDC extents directly to world extents. Summing the
    // absolute per-axis contributions gives the world AABB half size of one
    // cell: exact for the current axis-aligned light basis, conservative if the
    // light direction is ever tilted.
    const glm::mat3 lightToWorldLinear{lightViewProjToWorld};
    const glm::vec3 cellHalfSize =
        glm::abs(lightToWorldLinear * glm::vec3{1.0f / static_cast<float>(mapResolution[0]), 0.0f, 0.0f}) +
        glm::abs(lightToWorldLinear * glm::vec3{0.0f, 1.0f / static_cast<float>(mapResolution[1]), 0.0f}) +
        glm::abs(lightToWorldLinear * glm::vec3{0.0f, 0.0f, 0.5f / static_cast<float>(mapResolution[2])});

    WindowPlacements result;
    result.cellWorldSize = cellHalfSize * 2.0f;
    result.centerTexel = center;
    result.cells.reserve(WindowCellCount(windowSize));

    const std::array<int, 3> windowHalf{windowSize[0] / 2, windowSize[1] / 2, windowSize[2] / 2};

    for (size_t i = 0; i < WindowCellCount(windowSize); ++i)
    {
        const std::array<int, 3> offset = WindowOffsetAt(i, windowSize);
        const std::array<int, 3> texel{center[0] - windowHalf[0] + offset[0], center[1] - windowHalf[1] + offset[1],
                                       center[2] - windowHalf[2] + offset[2]};

        // Texel-center NDC, matching the precompute shader's texel -> NDC
        // mapping exactly.
        const glm::vec3 ndc{
            ((static_cast<float>(texel[0]) + 0.5f) / static_cast<float>(mapResolution[0])) * 2.0f - 1.0f,
            ((static_cast<float>(texel[1]) + 0.5f) / static_cast<float>(mapResolution[1])) * 2.0f - 1.0f,
            (static_cast<float>(texel[2]) + 0.5f) / static_cast<float>(mapResolution[2]),
        };

        const glm::vec4 world = lightViewProjToWorld * glm::vec4{ndc, 1.0f};

        result.cells.push_back(
            CellPlacement{.position = glm::vec3{world.x, world.y, world.z} / world.w, .size = result.cellWorldSize});
    }

    return result;
}
} // namespace transmittance_viz