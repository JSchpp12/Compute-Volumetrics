#include "render_system/fog/sync/VolumeSyncProvider.hpp"

#include <starlight/core/Exceptions.hpp>

vk::SemaphoreSubmitInfo render_system::fog::sync::VolumeSyncProvider::getSignalInfo() const
{
    if (!m_signalApproach.has_value())
        STAR_THROW("No sync approach was ever assigned");

    if (std::holds_alternative<signal::CalcFromFt>(m_signalApproach.value()))
        return std::get<signal::CalcFromFt>(m_signalApproach.value()).getSignalInfo();

    return vk::SemaphoreSubmitInfo();
}

render_system::fog::sync::WaitInfo render_system::fog::sync::VolumeSyncProvider::getWaitInfo() const
{
    if (!m_waitApproach.has_value())
        STAR_THROW("No wait approach was ever assigned");

    if (std::holds_alternative<wait::GatherFromCO>(m_waitApproach.value()))
        return std::get<wait::GatherFromCO>(m_waitApproach.value()).getWaitInfo();
    else if (std::holds_alternative<wait::WaitForPreviousChunk>(m_waitApproach.value()))
        return std::get<wait::WaitForPreviousChunk>(m_waitApproach.value()).getWaitInfo();

    return {};
}