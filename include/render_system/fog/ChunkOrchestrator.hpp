#pragma once

#include "FogType.hpp"
#include "render_system/FogDispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/FullPass.hpp"
#include "render_system/fog/sync/VolumeSyncProvider.hpp"

#include <starlight/core/device/DeviceContext.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog
{
class ChunkOrchestrator
{
    star::StarCommandBuffer m_cmdBuf{};
    commands::FullPass m_cmdApproach;
    sync::VolumeSyncProvider m_syncApproach;
    const bool *m_isReady{nullptr};

  public:
    ChunkOrchestrator() = default;
    ChunkOrchestrator(star::StarCommandBuffer cmdBuf, commands::FullPass cmdApproach,
                      sync::VolumeSyncProvider syncApproach, const bool *isReady)
        : m_cmdBuf(std::move(cmdBuf)), m_cmdApproach(std::move(cmdApproach)), m_syncApproach(std::move(syncApproach)),
          m_isReady(isReady)
    {
    }

    void recordCommands(const render_system::FogDispatchInfo &dInfo, const PassInfo &vInfo,
                        const PassPipelineInfo &pipeInfo, const star::common::FrameTracker &ft, Fog::Type type);

    void cleanupRender(star::core::device::DeviceContext &ctx);

    vk::SemaphoreSubmitInfo getSignalInfo(const star::common::FrameTracker &ft) const;

    sync::WaitInfo getWaitInfo(const star::common::FrameTracker &ft);

    vk::CommandBufferSubmitInfo getSubmitInfo(const star::common::FrameTracker &ft);
};
} // namespace render_system::fog