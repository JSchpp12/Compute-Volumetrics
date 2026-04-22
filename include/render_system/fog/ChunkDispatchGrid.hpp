#pragma once

#include "render_system/fog/ChunkOrchestrator.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <starlight/core/device/DeviceContext.hpp>

#include <vector>

namespace render_system::fog
{
class ChunkDispatchGrid
{
    std::array<uint8_t, 2> m_numChunksPerDimension{0, 0};
    std::array<uint32_t, 2> m_workgroupSize{0, 0};
    std::vector<render_system::fog::ChunkOrchestrator> m_chunkHandlers;
    const star::core::CommandBus *m_cmdBus{nullptr};
    static constexpr size_t NUM_GRID = 8;

    void createChunkHandlers(star::core::device::DeviceContext &ctx, star::Handle &passRegistration, bool &isReady);

  public:
    explicit ChunkDispatchGrid(std::array<uint8_t, 2> numChunksPerDimension)
        : m_numChunksPerDimension(std::move(numChunksPerDimension))
    {
    }

    void prepRender(star::core::device::DeviceContext &ctx, star::Handle &passRegistration, bool &isReady);

    void cleanupRender(star::core::device::DeviceContext &ctx);

    void recordAllChunks(const star::common::FrameTracker &ft, const PassInfo &pInfo, const PassPipelineInfo &pipeInfo,
                         Fog::Type type);

    void submitAllChunks(const star::common::FrameTracker &ft, std::vector<vk::Semaphore> dataSemaphores,
                         std::vector<vk::PipelineStageFlags> dataWaitPoints,
                         std::vector<std::optional<uint64_t>> previousSignaledValues, star::StarQueue &queue,
                         const star::Handle &registration);

    uint64_t getTimelineDoneSignalValue(const star::common::FrameTracker &ft) const;
};
} // namespace renderer::fog