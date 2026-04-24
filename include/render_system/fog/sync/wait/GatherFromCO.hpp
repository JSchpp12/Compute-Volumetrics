#pragma once

#include "render_system/fog/struct/SyncInfo.hpp"

#include <star_common/Handle.hpp>
#include <starlight/core/CommandBus.hpp>

namespace render_system::fog::sync::wait
{
class GatherFromCO
{
    star::Handle m_myRegistration;
    star::core::CommandBus *m_cmdBus{nullptr};

  public:
    GatherFromCO(star::Handle myRegistration, star::core::CommandBus *cmdBus)
        : m_myRegistration(std::move(myRegistration)), m_cmdBus(cmdBus)
    {
    }

    WaitInfo getWaitInfo() const;
};
} // namespace render_system::fog::sync::wait