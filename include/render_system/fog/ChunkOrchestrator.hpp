#pragma once

#include "FogType.hpp"
#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/FullPass.hpp"
#include "render_system/fog/sync/SyncProvider.hpp"

#include <starlight/core/device/DeviceContext.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog
{
class ChunkOrchestrator
{
    star::StarCommandBuffer m_cmdBuf{};
    commands::FullPass m_cmdApproach;
    sync::SyncProvider m_syncApproach;
    const bool *m_isReady{nullptr};

  public:
    ChunkOrchestrator() = default;
    ChunkOrchestrator(star::StarCommandBuffer cmdBuf, commands::FullPass cmdApproach, sync::SyncProvider syncApproach,
                      const bool *isReady)
        : m_cmdBuf(std::move(cmdBuf)), m_cmdApproach(std::move(cmdApproach)), m_syncApproach(std::move(syncApproach)),
          m_isReady(isReady)
    {
    }

    void recordCommands(const DispatchInfo &dInfo, const PassInfo &vInfo, const PassPipelineInfo &pipeInfo,
                        const star::common::FrameTracker &ft, Fog::Type type);

    void cleanupRender(star::core::device::DeviceContext &ctx);

    vk::SemaphoreSubmitInfo getSignalInfo(const star::common::FrameTracker &ft) const;

    WaitInfo getWaitInfo(const star::common::FrameTracker &ft);

    vk::CommandBufferSubmitInfo getSubmitInfo(const star::common::FrameTracker &ft);
};
} // namespace render_system::fog