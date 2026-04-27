#pragma once

#include "FogType.hpp"
#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/Pass.hpp"
#include "render_system/fog/sync/SyncProvider.hpp"

#include <starlight/core/device/DeviceContext.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog
{
class ChunkOrchestrator
{
    star::StarCommandBuffer m_cmdBuf{};
    std::vector<commands::Pass> m_cmdApproaches;
    std::optional<sync::SyncProvider> m_syncApproach{std::nullopt};
    uint32_t m_shaderOptionFlags;
    const bool *m_isReady{nullptr};

  public:
    ChunkOrchestrator() = default;
    ChunkOrchestrator(star::StarCommandBuffer cmdBuf, std::vector<commands::Pass> cmdApproaches, const bool *isReady)
        : m_cmdBuf(std::move(cmdBuf)), m_cmdApproaches(std::move(cmdApproaches)), m_syncApproach(std::nullopt),
          m_isReady(isReady)
    {
    }
    ChunkOrchestrator(star::StarCommandBuffer cmdBuf, std::vector<commands::Pass> cmdApproaches,
                      sync::SyncProvider syncApproach, const bool *isReady)
        : m_cmdBuf(std::move(cmdBuf)), m_cmdApproaches(std::move(cmdApproaches)),
          m_syncApproach(std::move(syncApproach)), m_isReady(isReady)
    {
    }

    void recordCommands(const DispatchInfo &dInfo, const PassInfo &vInfo, const PassPipelineInfo &pipeInfo,
                        const star::common::FrameTracker &ft);

    void cleanupRender(star::core::device::DeviceContext &ctx);

    [[nodiscard]] std::optional<vk::SemaphoreSubmitInfo> getSignalInfo(
        const star::common::FrameTracker &ft) const noexcept;

    [[nodiscard]] std::optional<WaitInfo> getWaitInfo(const star::common::FrameTracker &ft) noexcept;
    [[nodiscard]] const star::StarCommandBuffer &getCmdBuffer() const noexcept
    {
        return m_cmdBuf;
    }
};
} // namespace render_system::fog