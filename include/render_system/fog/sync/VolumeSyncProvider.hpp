#pragma once

#include "render_system/fog/sync/SyncInfo.hpp"
#include "render_system/fog/sync/signal/CalcFromFt.hpp"
#include "render_system/fog/sync/wait/GatherFromCO.hpp"
#include "render_system/fog/sync/wait/WaitForPreviousChunk.hpp"

#include <vulkan/vulkan.hpp>

#include <variant>

namespace render_system::fog::sync
{
class VolumeSyncProvider
{
    std::optional<std::variant<signal::CalcFromFt>> m_signalApproach{std::nullopt};
    std::optional<std::variant<wait::GatherFromCO, wait::WaitForPreviousChunk>> m_waitApproach{std::nullopt};

  public:
    VolumeSyncProvider() = default;
    VolumeSyncProvider(signal::CalcFromFt signalApproach, wait::GatherFromCO waitApproach)
        : m_signalApproach(std::move(signalApproach)), m_waitApproach(std::move(waitApproach))
    {
    }

    VolumeSyncProvider(signal::CalcFromFt signalApproach, wait::WaitForPreviousChunk waitApproach)
        : m_signalApproach(std::move(signalApproach)), m_waitApproach(std::move(waitApproach))
    {
    }

    vk::SemaphoreSubmitInfo getSignalInfo() const;

    WaitInfo getWaitInfo() const;
};
} // namespace render_system::fog::sync