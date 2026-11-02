#pragma once

#include "Volume.hpp"

#include <star_terrain/rendering/ShadowWindowBounds.hpp>

#include <star_common/FrameTracker.hpp>

#include <optional>
#include <vector>

/// @brief Tracks the fog volume's world-space bounds and tells the terrain shadow phase when they change.
class FogVolumeInfoController : public star::terrain::rendering::ShadowWindowBoundsProvider
{
  public:
    FogVolumeInfoController(const Volume &volume, uint8_t numFramesInFlight);

    virtual ~FogVolumeInfoController() = default;

    virtual star::terrain::rendering::BoundsDefinition getShadowWindowBounds() const override;

    virtual bool willShadowWindowBoundsUpdate(const star::common::FrameTracker &frameTracker) const override;

    virtual void markShadowWindowBoundsSynced(uint8_t frameInFlightIndex) override;

  private:
    star::terrain::rendering::BoundsDefinition calculateCurrentBounds() const;

    const Volume &m_volume;
    std::vector<std::optional<star::terrain::rendering::BoundsDefinition>> m_lastSyncedBounds;
};