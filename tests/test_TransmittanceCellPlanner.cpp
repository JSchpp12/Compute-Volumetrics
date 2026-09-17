#include "TransmittanceCellPlanner.hpp"

#include <starlight/virtual/StarCamera.hpp>

#include <glm/glm.hpp>

#include <gtest/gtest.h>

#include <cmath>

namespace
{
constexpr float kRelativeTolerance = 1.0e-4f;

transmittance_viz::WindowPlacements MakePlacements()
{
    star::StarCamera camera(1920, 1080, 90.0f, 1.0f, 25000.0f);
    return transmittance_viz::ComputeWindowPlacements(camera, glm::vec3{0.0f, -1.0f, 0.0f}, {64, 64, 32}, {4, 4, 4});
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