#pragma once

#include "FogType.hpp"
#include "render_system/FogDispatchInfo.hpp"
#include "renderer/FullPassVolumeCommands.hpp"
#include "renderer/VolumePassInfo.hpp"
#include "renderer/VolumeSyncProvider.hpp"

#include <starlight/core/device/DeviceContext.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog
{
class ChunkOrchestrator
{
    star::StarCommandBuffer m_cmdBuf{};
    renderer::FullPassVolumeCommands m_cmdApproach;
    renderer::VolumeSyncProvider m_syncApproach;
    const bool *m_isReady{nullptr};

  public:
    ChunkOrchestrator() = default;
    ChunkOrchestrator(star::StarCommandBuffer cmdBuf, renderer::FullPassVolumeCommands cmdApproach,
                         renderer::VolumeSyncProvider syncApproach, const bool *isReady)
        : m_cmdBuf(std::move(cmdBuf)), m_cmdApproach(std::move(cmdApproach)), m_syncApproach(std::move(syncApproach)),
          m_isReady(isReady)
    {
    }

    void recordCommands(const render_system::FogDispatchInfo &dInfo, const renderer::VolumePassInfo &vInfo,
                        const renderer::VolumePassPipelineInfo &pipeInfo, const star::common::FrameTracker &ft,
                        Fog::Type type);

    void cleanupRender(star::core::device::DeviceContext &ctx); 
    
    vk::SemaphoreSubmitInfo getSignalInfo(const star::common::FrameTracker &ft) const;

    renderer::VolumeWaitInfo getWaitInfo(const star::common::FrameTracker &ft);

    vk::CommandBufferSubmitInfo getSubmitInfo(const star::common::FrameTracker &ft);
};
} // namespace render_system