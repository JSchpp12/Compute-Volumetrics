#pragma once

#include "render_system/fog/ChunkOrchestrator.hpp"
#include "render_system/fog/sync/SyncProvider.hpp"

#include <starlight/core/device/DeviceContext.hpp>

namespace render_system::fog
{
class FogDispatcher
{
  public:
    struct DispatchContextInfo
    {
        vk::Extent2D engineResolution;
        vk::Extent3D tranmittanceTextureResolution;
    };

    void prepRender(star::core::device::DeviceContext &ctx, star::Handle &passReg, DispatchContextInfo contextInfo,
                    bool &isReady);

    void cleanupRender(star::core::device::DeviceContext &ctx);

    void submit(const star::common::FrameTracker &ft, std::vector<vk::Semaphore> dataSemaphores,
                std::vector<vk::PipelineStageFlags> dataWaitPoints,
                std::vector<std::optional<uint64_t>> previousSignaledValues, star::StarQueue &queue,
                const star::Handle &registration);

    void recordCommands(DispatchInfo &dInfo, const star::common::FrameTracker &ft, const PassInfo &pInfo,
                        const PassPipelineInfo &pipeInfo);

    [[nodiscard]] uint64_t getTimelineDoneSignalValue(const star::common::FrameTracker &ft) const;

  private:
    std::vector<ChunkOrchestrator> m_passes;
    std::vector<vk::SemaphoreSubmitInfo> m_cbSubmitWait{6};
    std::vector<vk::CommandBufferSubmitInfo> m_cbSubmitInfo;
    sync::SyncProvider m_syncApproach;
    star::core::CommandBus *m_cmdBus{nullptr};
    uint8_t m_numCbRecorded{0};

    void createChunks(star::core::device::DeviceContext &ctx, star::Handle &passReg,
                      const DispatchContextInfo &dContext, bool &isReady);
};
} // namespace render_system::fog