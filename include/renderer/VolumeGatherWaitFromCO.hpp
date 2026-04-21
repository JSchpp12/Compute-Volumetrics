#pragma once

#include "renderer/VolumeSyncInfo.hpp"

#include <star_common/Handle.hpp>
#include <starlight/core/CommandBus.hpp>

namespace renderer
{
class VolumeGatherWaitFromCO
{
    star::Handle m_myRegistration;
    star::core::CommandBus *m_cmdBus{nullptr};

  public:
    VolumeGatherWaitFromCO(star::Handle myRegistration, star::core::CommandBus *cmdBus)
        : m_myRegistration(std::move(myRegistration)), m_cmdBus(cmdBus)
    {
    }

    renderer::VolumeWaitInfo getWaitInfo() const; 
};
} // namespace renderer