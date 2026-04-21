#pragma once

#include "renderer/VolumeGatherWaitFromCO.hpp"
#include "renderer/VolumeSyncCalcFromFt.hpp"
#include "renderer/VolumeSyncInfo.hpp"
#include "renderer/VolumeWaitForPreviousChunk.hpp"

#include <vulkan/vulkan.hpp>

#include <variant>

namespace renderer
{
class VolumeSyncProvider
{
    std::optional<std::variant<VolumeSyncCalcFromFt>> m_signalApproach{std::nullopt};
    std::optional<std::variant<VolumeGatherWaitFromCO, VolumeWaitForPreviousChunk>> m_waitApproach{std::nullopt};

  public:
    VolumeSyncProvider() = default;
    VolumeSyncProvider(VolumeSyncCalcFromFt signalApproach, VolumeGatherWaitFromCO waitApproach)
        : m_signalApproach(std::move(signalApproach)), m_waitApproach(std::move(waitApproach))
    {
    }

    VolumeSyncProvider(VolumeSyncCalcFromFt signalApproach, VolumeWaitForPreviousChunk waitApproach)
        : m_signalApproach(std::move(signalApproach)), m_waitApproach(std::move(waitApproach))
    {
    }

    vk::SemaphoreSubmitInfo getSignalInfo() const;

    renderer::VolumeWaitInfo getWaitInfo() const;
};
} // namespace renderer