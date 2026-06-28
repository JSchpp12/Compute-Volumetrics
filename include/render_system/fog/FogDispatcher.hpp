#pragma once

#include "render_system/fog/ChunkOrchestrator.hpp"
#include "render_system/fog/sync/SyncProvider.hpp"

#include <starlight/core/device/DeviceContext.hpp>

namespace render_system::fog
{
class FogDispatcherBuilder;

class FogDispatcher
{
  public:
    FogDispatcher() = default;
    explicit FogDispatcher(bool enableColorDebugCutoff)
        : m_passes(), m_cbSubmitInfo(), m_syncApproach(), m_cmdBus(nullptr), m_numCbRecorded(0),
          m_enableColorDebugCutoff(enableColorDebugCutoff)
    {
    }

    void prepRender(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady);

    void cleanupRender(star::core::device::DeviceContext &ctx);

    void submit(const star::common::FrameTracker &ft, std::vector<vk::Semaphore> dataSemaphores,
                std::vector<vk::PipelineStageFlags> dataWaitPoints,
                std::vector<std::optional<uint64_t>> previousSignaledValues, star::StarQueue &queue,
                const star::Handle &registration);

    void recordCommands(DispatchInfo &dInfo, const star::common::FrameTracker &ft, const PassInfo &pInfo,
                        const PassPipelineInfo &pipeInfo);

    [[nodiscard]] uint64_t getTimelineDoneSignalValue(const star::common::FrameTracker &ft) const;

  private:
    friend class FogDispatcherBuilder;
    std::vector<ChunkOrchestrator> m_passes;
    std::vector<vk::CommandBufferSubmitInfo> m_cbSubmitInfo;
    sync::SyncProvider m_syncApproach;
    star::core::CommandBus *m_cmdBus{nullptr};
    uint8_t m_numCbRecorded{0};
    bool m_enableColorDebugCutoff{false};

    void createChunks(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady);
};
} // namespace render_system::fog