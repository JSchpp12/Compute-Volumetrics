#pragma once

#include <star_terrain/file_data/ChunkInfo.hpp>
#include <star_terrain/io/IFilter.hpp>

#include <glm/glm.hpp>

namespace filter
{
/// @brief Ground sector a filter keeps, in the terrain's local world frame (x = north, y = east).
struct FilterRange
{
    glm::dvec2 center{};
    double viewDistance{};
    /// @brief Azimuth of the sector's leading edge in radians, measured from +Y (east) towards +X (north).
    double startAzimuth{};
    /// @brief Angular width of the sector in radians.
    double sweptAngle{};
};

/// @brief Keeps only the terrain chunks inside a ground sector.
/// @details Chunks store geographic coordinates, so their corners are projected into the terrain's local world frame
/// (x = north, y = up, z = east) before the test.
class SimulationViewCoverageFilter : public star::terrain::IFilter
{
  public:
    /// @param range Ground sector to keep, in the terrain's local world frame (x = north, y = east).
    /// @param terrainOriginLatLon Geographic (lat, lon) of the terrain's local origin, i.e. its shape center.
    SimulationViewCoverageFilter(const FilterRange &range, glm::dvec2 terrainOriginLatLon);

    [[nodiscard]] bool shouldKeep(const star::terrain::ChunkInfo &chunk) const override;

  private:
    [[nodiscard]] bool isInsideView(const glm::dvec2 &worldPosition) const;

    [[nodiscard]] glm::dvec2 toWorld(const glm::dvec2 &latLon) const;

    FilterRange m_range;
    glm::dvec3 m_originEcef;
    glm::dmat3 m_ecefToEnu;
};
} // namespace filter
