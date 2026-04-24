#include "render_system/fog/FogDispatcher.hpp"

#include "render_system/fog/commands/Distance.hpp"
#include "render_system/fog/commands/FullPass.hpp"
#include "render_system/fog/commands/PostMemoryBarrierDifferentFamilies.hpp"
#include "render_system/fog/commands/PreMemoryBarrierDifferentFamilies.hpp"
#include "render_system/fog/struct/SyncInfo.hpp"

#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

using namespace render_system::fog;
using namespace render_system::fog::commands;
using namespace render_system::fog::sync;

namespace render_system::fog
{
static std::tuple<uint32_t, uint32_t, uint32_t> GetQueueFamilyIndices(star::core::device::DeviceContext &ctx)
{
    const auto graphicsQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(ctx.getEventBus(), ctx.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();

    const auto computeQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(ctx.getEventBus(), ctx.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tcompute)
            ->getParentQueueFamilyIndex();

    const auto transferQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(ctx.getEventBus(), ctx.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Ttransfer)
            ->getParentQueueFamilyIndex();

    return std::make_tuple(graphicsQueueFamilyIndex, computeQueueFamilyIndex, transferQueueFamilyIndex);
}

static std::array<uint32_t, 2> CalculateWorkgroupSize(const vk::Extent2D &screensize)
{
    const uint32_t width = static_cast<uint32_t>(std::ceil(static_cast<float>(screensize.width) / 8.0f));
    const uint32_t height = static_cast<uint32_t>(std::ceil(static_cast<float>(screensize.height) / 8.0f));

    return {width, height};
}

void FogDispatcher::prepRender(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady)
{
    m_cmdBus = &ctx.getCmdBus();

    m_workgroupSize = CalculateWorkgroupSize(ctx.getEngineResolution());

    createChunks(ctx, passReg, isReady);
}

void FogDispatcher::cleanupRender(star::core::device::DeviceContext &ctx)
{
    for (auto &chunk : m_chunks)
    {
        chunk.cleanupRender(ctx);
    }
}

void FogDispatcher::submit(const star::common::FrameTracker &ft, std::vector<vk::Semaphore> dataSemaphores,
                           std::vector<vk::PipelineStageFlags> dataWaitPoints,
                           std::vector<std::optional<uint64_t>> previousSignaledValues, star::StarQueue &queue,
                           const star::Handle &registration)
{
    assert(m_chunks.size() > 0);

    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
    std::vector<vk::CommandBufferSubmitInfo> cbInfo;
    cbInfo.resize(m_chunks.size());
    std::vector<WaitInfo> chunkWaitInfos;
    chunkWaitInfos.resize(m_chunks.size());
    std::vector<vk::SemaphoreSubmitInfo> chunkSignalInfos;
    chunkSignalInfos.resize(m_chunks.size());

    {
        auto gCmd = star::command_order::GetPassInfo{registration};
        m_cmdBus->submit(gCmd);
        const auto workingSemaphore = gCmd.getReply().get().signaledSemaphore;
        for (size_t i{0}; i < m_chunks.size(); i++)
        {
            cbInfo[i] = m_chunks[i].getSubmitInfo(ft);

            chunkWaitInfos[i] = m_chunks[i].getWaitInfo(ft);
            // inject whole volume system semaphore to wait infos missing one
            // assuming that these are the approaches which are waiting on a previous
            // chunk to complete its work
            for (size_t j = 0; j < chunkWaitInfos[i].count; j++)
            {
                if (!chunkWaitInfos[i].info[j].semaphore)
                    chunkWaitInfos[i].info[j].semaphore = workingSemaphore;
            }

            chunkSignalInfos[i] = m_chunks[i].getSignalInfo(ft);
            chunkSignalInfos[i].setSemaphore(workingSemaphore);
        }
    }

    assert(dataSemaphores.size() == dataWaitPoints.size());
    assert(dataWaitPoints.size() + chunkWaitInfos[0].count < 5 &&
           "Current implementation of wait info container only allows 5");

    for (size_t i{0}; i < dataWaitPoints.size(); i++)
    {
        auto &w = chunkWaitInfos[0];
        w.info[w.count] = vk::SemaphoreSubmitInfo()
                              .setSemaphore(dataSemaphores[i])
                              .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

        w.count++;
    }

    {
        std::vector<std::array<vk::SemaphoreSubmitInfo, 5>> waitInfo;
        waitInfo.resize(m_chunks.size());
        std::vector<uint32_t> waitInfoCount;
        waitInfoCount.resize(m_chunks.size());
        std::vector<vk::SubmitInfo2> submitInfo;
        submitInfo.resize(m_chunks.size());

        for (int i{0}; i < m_chunks.size(); i++)
        {
            // process wait infos
            for (int j{0}; j < 5; j++)
            {
                if (j < chunkWaitInfos[i].count)
                    waitInfo[i][j] = std::move(chunkWaitInfos[i].info[j]);
            }

            submitInfo[i]
                .setPSignalSemaphoreInfos(&chunkSignalInfos[i])
                .setSignalSemaphoreInfoCount(1)
                .setPCommandBufferInfos(&cbInfo[i])
                .setCommandBufferInfoCount(1)
                .setPWaitSemaphoreInfos(waitInfo[i].data())
                .setWaitSemaphoreInfoCount(chunkWaitInfos[i].count);
        }

        queue.getVulkanQueue().submit2(submitInfo);
    }
}
void FogDispatcher::recordCommands(const star::common::FrameTracker &ft, const PassInfo &pInfo,
                                   const PassPipelineInfo &pipeInfo, Fog::Type type)
{
    assert(m_chunks.size() > 0);

    DispatchInfo dInfo{.workgroupSize = {m_workgroupSize[0], m_workgroupSize[1]}, .localThreadGroupSize = {8, 8}};

    for (auto &chunk : m_chunks)
    {
        chunk.recordCommands(dInfo, pInfo, pipeInfo, ft, type);
    }
}
uint64_t FogDispatcher::getTimelineDoneSignalValue(const star::common::FrameTracker &ft) const
{
    return m_chunks.back().getSignalInfo(ft).value;
}

void FogDispatcher::createChunks(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady)
{
    const size_t nf = static_cast<size_t>(ctx.frameTracker().getSetup().getNumFramesInFlight());

    const auto [graphicsQueueFamilyIndex, computeQueueFamilyIndex, transferQueueFamilyIndex] =
        GetQueueFamilyIndices(ctx);
    const auto *queueInfo = ctx.getManagerCommandBuffer().m_manager.getInUseInfoForType(star::Queue_Type::Tcompute);
    assert(queueInfo != nullptr && "Failed to get queue info from manager");

    m_chunks.resize(1);
    m_chunks[0] = render_system::fog::ChunkOrchestrator{
        star::StarCommandBuffer{ctx.getDevice().getVulkanDevice(), static_cast<int>(nf), &queueInfo->pool,
                                star::Queue_Type::Tcompute, false, false},
        FullPass{ComputeContributor{Color{}},
                 PreMemoryBarrierContributor{PreMemoryBarrierDifferentFamilies{
                     computeQueueFamilyIndex, graphicsQueueFamilyIndex, transferQueueFamilyIndex}},
                 PostMemoryBarrierContributor{PostMemoryBarrierDifferentFamilies{
                     computeQueueFamilyIndex, graphicsQueueFamilyIndex, transferQueueFamilyIndex}}},
        SyncProvider{signal::CalcFromFt(0, 1, &ctx.frameTracker()), wait::GatherFromCO{passReg, &ctx.getCmdBus()}},
        &isReady};
}
} // namespace render_system::fog