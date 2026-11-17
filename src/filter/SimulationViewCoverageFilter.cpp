#include "filter/SimulationViewCoverageFilter.hpp"

#include <star_terrain/util/Distance.hpp>

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

namespace filter
{
SimulationViewCoverageFilter::SimulationViewCoverageFilter(const FilterRange &range, glm::dvec2 terrainOriginLatLon)
    : m_range(range),
      m_originEcef(star::terrain::util::distance::toECEF(terrainOriginLatLon.x, terrainOriginLatLon.y, 0.0)),
      m_ecefToEnu(
          star::terrain::util::distance::getECEFToENUTransformation(terrainOriginLatLon.x, terrainOriginLatLon.y))
{
}

bool SimulationViewCoverageFilter::shouldKeep(const star::terrain::ChunkInfo &chunk) const
{
    const glm::dvec2 corners[] = {toWorld(chunk.cornerNE), toWorld(chunk.cornerSE), toWorld(chunk.cornerSW),
                                  toWorld(chunk.cornerNW)};

    for (const auto &corner : corners)
    {
        if (isInsideView(corner))
        {
            return true;
        }
    }

    if (isInsideView(toWorld(chunk.center)))
    {
        return true;
    }

    // A sector can pass through a chunk without entering any of its corners, so keep chunks whose world bounds contain
    // the sector's origin.
    const auto [minX, maxX] = std::minmax({corners[0].x, corners[1].x, corners[2].x, corners[3].x});
    const auto [minZ, maxZ] = std::minmax({corners[0].y, corners[1].y, corners[2].y, corners[3].y});
    return m_range.center.x >= minX && m_range.center.x <= maxX && m_range.center.y >= minZ && m_range.center.y <= maxZ;
}

bool SimulationViewCoverageFilter::isInsideView(const glm::dvec2 &worldPosition) const
{
    const glm::dvec2 offset = worldPosition - m_range.center;
    if (glm::dot(offset, offset) > m_range.viewDistance * m_range.viewDistance)
    {
        return false;
    }

    const double fullTurn = glm::two_pi<double>();
    if (m_range.sweptAngle >= fullTurn)
    {
        return true;
    }

    // The view sweeps in the direction of increasing azimuth, so a position is covered when its offset from the
    // sector's leading edge stays within the swept angle.
    double relativeAzimuth = std::fmod(std::atan2(offset.x, offset.y) - m_range.startAzimuth, fullTurn);
    if (relativeAzimuth < 0.0)
    {
        relativeAzimuth += fullTurn;
    }

    return relativeAzimuth <= m_range.sweptAngle;
}

glm::dvec2 SimulationViewCoverageFilter::toWorld(const glm::dvec2 &latLon) const
{
    const glm::dvec3 ecef = star::terrain::util::distance::toECEF(latLon.x, latLon.y, 0.0);

    // Matches LoadFullGeometry::centerAroundTerrainOrigin: the transformation yields (east, north, up), and terrain
    // world positions are (north, up, east). Following the same transform approach here.
    const glm::dvec3 enu = m_ecefToEnu * (ecef - m_originEcef);
    return {enu.y, enu.x};
}
} // namespace filter
