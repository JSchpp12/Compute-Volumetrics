#include "renderer/VolumeSyncProvider.hpp"

#include <starlight/core/Exceptions.hpp>

vk::SemaphoreSubmitInfo renderer::VolumeSyncProvider::getSignalInfo() const
{
    if (!m_signalApproach.has_value())
        STAR_THROW("No sync approach was ever assigned");

    if (std::holds_alternative<VolumeSyncCalcFromFt>(m_signalApproach.value()))
        return std::get<VolumeSyncCalcFromFt>(m_signalApproach.value()).getSignalInfo();

    return vk::SemaphoreSubmitInfo();
}

renderer::VolumeWaitInfo renderer::VolumeSyncProvider::getWaitInfo() const
{
    if (!m_waitApproach.has_value())
        STAR_THROW("No wait approach was ever assigned");

    if (std::holds_alternative<VolumeGatherWaitFromCO>(m_waitApproach.value()))
        return std::get<VolumeGatherWaitFromCO>(m_waitApproach.value()).getWaitInfo();
    else if (std::holds_alternative<VolumeWaitForPreviousChunk>(m_waitApproach.value()))
        return std::get<VolumeWaitForPreviousChunk>(m_waitApproach.value()).getWaitInfo();

    return {};
}