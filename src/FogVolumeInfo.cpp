#include "FogVolumeInfo.hpp"

#include <cassert>
#include <limits>

FogVolumeInfoController::FogVolumeInfoController(const Volume &volume, uint8_t numFramesInFlight)
    : m_volume(volume), m_lastSyncedBounds(static_cast<size_t>(numFramesInFlight), std::nullopt)
{
}

bool FogVolumeInfoController::getShadowWindowBounds(star::terrain::rendering::BoundsDefinition &out) const
{
    out = calculateCurrentBounds();
    return true;
}

bool FogVolumeInfoController::willShadowWindowBoundsUpdate(const star::common::FrameTracker &frameTracker) const
{
    const size_t frameInFlightIndex = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    assert(frameInFlightIndex < m_lastSyncedBounds.size() && "Not enough resources were created for this");

    const auto &lastSynced = m_lastSyncedBounds[frameInFlightIndex];
    if (!lastSynced.has_value())
    {
        // Nothing synced yet: always dirty, so the shadow window is fitted to the volume before
        // anything renders.
        return true;
    }

    return !(*lastSynced == calculateCurrentBounds());
}

void FogVolumeInfoController::markShadowWindowBoundsSynced(uint8_t frameInFlightIndex)
{
    assert(static_cast<size_t>(frameInFlightIndex) < m_lastSyncedBounds.size() &&
           "Not enough resources were created for this");

    m_lastSyncedBounds[frameInFlightIndex] = calculateCurrentBounds();
}

star::terrain::rendering::BoundsDefinition FogVolumeInfoController::calculateCurrentBounds() const
{
    // The volume's object-local space is the NanoVDB grid WORLD space, and the GPU transform used
    // by the precompute is the instance display matrix -- so the world AABB is the display matrix
    // applied to the grid world AABB corners.
    const auto &gridWorldAABB = m_volume.getGridWorldAABB();
    const glm::mat4 displayMatrix = m_volume.getInstance(0).getDisplayMatrix();

    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{-std::numeric_limits<float>::max()};
    for (int corner = 0; corner < 8; ++corner)
    {
        const glm::vec3 localCorner{((corner & 1) != 0) ? gridWorldAABB[1].x : gridWorldAABB[0].x,
                                    ((corner & 2) != 0) ? gridWorldAABB[1].y : gridWorldAABB[0].y,
                                    ((corner & 4) != 0) ? gridWorldAABB[1].z : gridWorldAABB[0].z};
        const glm::vec3 worldCorner = glm::vec3(displayMatrix * glm::vec4(localCorner, 1.0f));
        min = glm::min(min, worldCorner);
        max = glm::max(max, worldCorner);
    }

    return star::terrain::rendering::BoundsDefinition::Builder().asBox(min, max).build();
}