#pragma once

#include "render_system/fog/struct/SyncInfo.hpp"
#include "render_system/fog/sync/signal/CalcFromFt.hpp"
#include "render_system/fog/sync/wait/GatherFromCO.hpp"
#include "render_system/fog/sync/wait/WaitForPreviousChunk.hpp"
#include "render_system/fog/sync/wait/WaitForFirstChunk.hpp"

#include <vulkan/vulkan.hpp>

#include <variant>

namespace render_system::fog::sync
{
class SyncProvider
{
    std::optional<std::variant<signal::CalcFromFt>> m_signalApproach{std::nullopt};
    std::optional<std::variant<wait::GatherFromCO, wait::WaitForPreviousChunk, wait::WaitForFirstChunk>> m_waitApproach{std::nullopt};

  public:
    SyncProvider() = default;
    SyncProvider(signal::CalcFromFt signalApproach, wait::GatherFromCO waitApproach)
        : m_signalApproach(std::move(signalApproach)), m_waitApproach(std::move(waitApproach))
    {
    }

    SyncProvider(signal::CalcFromFt signalApproach, wait::WaitForPreviousChunk waitApproach)
        : m_signalApproach(std::move(signalApproach)), m_waitApproach(std::move(waitApproach))
    {
    }
    SyncProvider(signal::CalcFromFt signalApproach, wait::WaitForFirstChunk waitApproach)
        : m_signalApproach(std::move(signalApproach)), m_waitApproach(std::move(waitApproach))
    {
    }

    vk::SemaphoreSubmitInfo getSignalInfo() const;

    WaitInfo getWaitInfo() const;
};
} // namespace render_system::fog::sync