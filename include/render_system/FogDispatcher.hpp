#pragma once

#include "OffscreenRenderer.hpp"
#include "renderer/VolumeRenderer.hpp"

#include <starlight/core/device/DeviceContext.hpp>

namespace render_system
{
class FogDispatcherBuilder;

class FogDispatcher
{
  public:
    void prepRender(star::core::device::DeviceContext &ctx);

    void frameUpdate(const star::core::device::DeviceContext &ctx);

  private:
    friend class FogDispatcherBuilder;
    OffscreenRenderer *m_terrainCmds{nullptr};
    VolumeRenderer *m_volumeCmds{nullptr};

    FogDispatcher(OffscreenRenderer *terrainCmds, VolumeRenderer *volumeCmds)
        : m_terrainCmds{terrainCmds}, m_volumeCmds{volumeCmds} {};
};
} // namespace render_system