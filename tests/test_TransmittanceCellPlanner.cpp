#include "TransmittanceCellPlanner.hpp"

#include <starlight/virtual/StarCamera.hpp>

#include <glm/glm.hpp>

#include <gtest/gtest.h>

#include <cmath>

namespace
{
constexpr float kRelativeTolerance = 1.0e-4f;

/// Axis-aligned volume bounds of the given half extent, as FogVolumeInfoController reports.
star::terrain::rendering::BoundsDefinition MakeBounds(const float halfExtent)
{
    return star::terrain::rendering::BoundsDefinition::Builder()
        .asBox(glm::vec3{-halfExtent, -halfExtent, -halfExtent}, glm::vec3{halfExtent, halfExtent, halfExtent})
        .build();
}

transmittance_viz::WindowPlacements MakePlacementsFor(const float halfExtent)
{
    star::StarCamera camera(1920, 1080, 90.0f, 1.0f, 25000.0f);
    const glm::mat4 worldToLightViewProj = transmittance_viz::ComputeLightViewProj(
        camera, glm::vec3{0.0f, -1.0f, 0.0f}, MakeBounds(halfExtent));

    return transmittance_viz::ComputeWindowPlacements(worldToLightViewProj, camera.getPosition(), {64, 64, 32},
                                                       {4, 4, 4});
}

transmittance_viz::WindowPlacements MakePlacements()
{
    return MakePlacementsFor(1000.0f);
}

/// Expect `actual` to be zero, within a tolerance scaled to the cell size
/// along the same world axis.
void ExpectAxisZero(float actual, float cellSizeAlongAxis)
{
    EXPECT_NEAR(actual, 0.0f, cellSizeAlongAxis * kRelativeTolerance);
}
} // namespace

TEST(TransmittanceCellPlanner, WindowHasOnePlacementPerCell)
{
    const auto placements = MakePlacements();

    EXPECT_EQ(placements.cells.size(), transmittance_viz::WindowCellCount({4, 4, 4}));
}

TEST(TransmittanceCellPlanner, EveryCellSharesTheGridSize)
{
    const auto placements = MakePlacements();

    for (const auto &cell : placements.cells)
    {
        EXPECT_NEAR(cell.size.x, placements.cellWorldSize.x, placements.cellWorldSize.x * kRelativeTolerance);
        EXPECT_NEAR(cell.size.y, placements.cellWorldSize.y, placements.cellWorldSize.y * kRelativeTolerance);
        EXPECT_NEAR(cell.size.z, placements.cellWorldSize.z, placements.cellWorldSize.z * kRelativeTolerance);
    }
}

/// The map's world extent is fitted to the volume's world bounds, so the texel cells have to
/// grow with the volume -- this is the behaviour the precompute actually has.
TEST(TransmittanceCellPlanner, CellSizeFollowsTheVolumeBounds)
{
    const auto small = MakePlacementsFor(500.0f);
    const auto large = MakePlacementsFor(1000.0f);

    EXPECT_GT(large.cellWorldSize.x, small.cellWorldSize.x);
    EXPECT_GT(large.cellWorldSize.y, small.cellWorldSize.y);
    EXPECT_GT(large.cellWorldSize.z, small.cellWorldSize.z);
}

TEST(TransmittanceCellPlanner, AdjacentCellsAreExactlyOneCellApart)
{
    const auto placements = MakePlacements();
    const auto &cells = placements.cells;
    const glm::vec3 size = placements.cellWorldSize;

    ASSERT_EQ(cells.size(), 64u);

    // With the straight-down light the map axes are world aligned: map x runs
    // along world X, map y along world Z, and map z (depth) along world Y.
    // Window index = z * 16 + y * 4 + x.
    const glm::vec3 xStep = cells[1].position - cells[0].position;
    EXPECT_NEAR(std::abs(xStep.x), size.x, size.x * kRelativeTolerance);
    ExpectAxisZero(xStep.y, size.y);
    ExpectAxisZero(xStep.z, size.z);

    const glm::vec3 yStep = cells[4].position - cells[0].position;
    ExpectAxisZero(yStep.x, size.x);
    ExpectAxisZero(yStep.y, size.y);
    EXPECT_NEAR(std::abs(yStep.z), size.z, size.z * kRelativeTolerance);

    const glm::vec3 zStep = cells[16].position - cells[0].position;
    ExpectAxisZero(zStep.x, size.x);
    EXPECT_NEAR(std::abs(zStep.y), size.y, size.y * kRelativeTolerance);
    ExpectAxisZero(zStep.z, size.z);
}