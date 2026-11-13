#pragma once

#include "render_system/fog/ChunkOrchestrator.hpp"
#include "render_system/fog/sync/SyncProvider.hpp"

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/wrappers/graphics/StarCommandBuffer.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace star::common
{
class FrameTracker;
}

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

    void submit(const star::common::FrameTracker &ft, std::span<const star::StarCommandBuffer::Data> dataWaits,
                star::StarQueue &queue, const star::Handle &registration);

    void recordCommands(DispatchInfo &dInfo, const star::common::FrameTracker &ft, PassInfo &pInfo,
                        const PassPipelineInfo &pipeInfo);

    [[nodiscard]] uint64_t getTimelineDoneSignalValue(const star::common::FrameTracker &ft) const;

    /// @brief Append one submission's wait list: DAG-edge waits followed by transfer data waits.
    /// @details Exposed for unit testing. Transfer data waits must carry their timeline value (`signaledValue`); a
    /// binary wait leaves it absent and submits value 0.
    static void AppendWaitInfos(std::vector<vk::SemaphoreSubmitInfo> &out, const WaitInfo &edgeWaits,
                                std::span<const star::StarCommandBuffer::Data> dataWaits);

  private:
    std::vector<ChunkOrchestrator> m_passes;
    star::StarCommandBuffer m_sharedCmdBuf;
    std::vector<vk::SemaphoreSubmitInfo> m_cbSubmitWait;
    sync::SyncProvider m_syncApproach;
    star::core::CommandBus *m_cmdBus{nullptr};
    uint8_t m_numCbRecorded{0};

    void createChunks(star::core::device::DeviceContext &ctx, star::Handle &passReg,
                      const DispatchContextInfo &dContext, bool &isReady);
};
} // namespace render_system::fog