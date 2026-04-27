#include "render_system/fog/FogDispatcher.hpp"

#include "render_system/fog/commands/Distance.hpp"
#include "render_system/fog/commands/Pass.hpp"
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

void FogDispatcher::prepRender(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady)
{
    m_cmdBus = &ctx.getCmdBus();

    const auto *queueInfo = ctx.getManagerCommandBuffer().m_manager.getInUseInfoForType(star::Queue_Type::Tcompute);
    assert(queueInfo != nullptr && "Failed to get queue info from manager");

    m_syncApproach = {signal::CalcFromFt{1, 1, &ctx.frameTracker()}, wait::GatherFromCO{passReg, &ctx.getCmdBus()}};

    createChunks(ctx, passReg, isReady);
}

void FogDispatcher::cleanupRender(star::core::device::DeviceContext &ctx)
{
    for (auto &chunk : m_passes)
    {
        chunk.cleanupRender(ctx);
    }
}

void FogDispatcher::submit(const star::common::FrameTracker &ft, std::vector<vk::Semaphore> dataSemaphores,
                           std::vector<vk::PipelineStageFlags> dataWaitPoints,
                           std::vector<std::optional<uint64_t>> previousSignaledValues, star::StarQueue &queue,
                           const star::Handle &registration)
{
    assert(m_passes.size() > 0);

    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
    std::vector<vk::CommandBufferSubmitInfo> cbInfo;
    cbInfo.resize(m_passes.size());

    auto gCmd = star::command_order::GetPassInfo{registration};
    m_cmdBus->submit(gCmd);
    const auto workingSemaphore = gCmd.getReply().get().signaledSemaphore;
    for (size_t i{0}; i < m_passes.size(); i++)
    {
        cbInfo[i] = vk::CommandBufferSubmitInfo().setCommandBuffer(m_passes[i].getCmdBuffer().buffer(ii));
    }

    assert(dataSemaphores.size() == dataWaitPoints.size());

    uint32_t waitInfoCount{0};
    vk::SemaphoreSubmitInfo waitInfo[5];
    {
        auto wait = m_syncApproach.getWaitInfo();
        for (uint8_t i{0}; i < wait.count; i++)
        {
            waitInfo[waitInfoCount] = wait.info[i];
            waitInfoCount++;
        }
    }

    for (size_t i{0}; i < dataWaitPoints.size(); i++)
    {
        waitInfo[waitInfoCount] = vk::SemaphoreSubmitInfo()
                                      .setSemaphore(dataSemaphores[i])
                                      .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
        waitInfoCount++;
    }

    // submitInfo.resize(1);

    vk::SemaphoreSubmitInfo signalInfo = vk::SemaphoreSubmitInfo()
                                             .setSemaphore(workingSemaphore)
                                             .setValue(getTimelineDoneSignalValue(ft))
                                             .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

    {
        vk::SubmitInfo2 submitInfo = vk::SubmitInfo2()
                                        .setPSignalSemaphoreInfos(&signalInfo)
                                        .setSignalSemaphoreInfoCount(1)
                                        .setPCommandBufferInfos(cbInfo.data())
                                        .setCommandBufferInfoCount(cbInfo.size())
                                        .setPWaitSemaphoreInfos(waitInfo)
                                        .setWaitSemaphoreInfoCount(waitInfoCount);

        queue.getVulkanQueue().submit2(submitInfo);
    }
}
void FogDispatcher::recordCommands(const DispatchInfo &dInfo, const star::common::FrameTracker &ft,
                                   const PassInfo &pInfo, const PassPipelineInfo &pipeInfo, Fog::Type type)
{
    assert(m_passes.size() > 0);

    // TODO: move the wait for semaphore value from the volume renderer to here

    for (auto &chunk : m_passes)
    {
        chunk.recordCommands(dInfo, pInfo, pipeInfo, ft, type);
    }
}
uint64_t FogDispatcher::getTimelineDoneSignalValue(const star::common::FrameTracker &ft) const
{
    return m_syncApproach.getSignalInfo().value;
}

static ChunkOrchestrator CreateColorPass(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady)
{
    const auto [graphicsQueueFamilyIndex, computeQueueFamilyIndex, transferQueueFamilyIndex] =
        GetQueueFamilyIndices(ctx);

    std::vector<commands::Pass> pass;
    pass.resize(3);

    const auto *queueInfo = ctx.getManagerCommandBuffer().m_manager.getInUseInfoForType(star::Queue_Type::Tcompute);
    assert(queueInfo != nullptr && "Failed to get queue info from manager");

    pass[0] = Pass{ComputeContributor{Init{ctx.getEngineResolution()}},
                   PreMemoryBarrierContributor{PreMemoryBarrierDifferentFamilies{
                       computeQueueFamilyIndex, graphicsQueueFamilyIndex, transferQueueFamilyIndex}}};

    pass[1] = Pass{ComputeContributor{IndirectDispatch{}}};

    pass[2] = Pass{ComputeContributor{Color{}},
                   PostMemoryBarrierContributor{PostMemoryBarrierDifferentFamilies{
                       computeQueueFamilyIndex, graphicsQueueFamilyIndex, transferQueueFamilyIndex}}};

    return ChunkOrchestrator{
        star::StarCommandBuffer(ctx.getDevice().getVulkanDevice(),
                                static_cast<int>(ctx.frameTracker().getSetup().getNumFramesInFlight()),
                                &queueInfo->pool, star::Queue_Type::Tcompute, false, false),
        std::move(pass), &isReady};
}

void FogDispatcher::createChunks(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady)
{
    const size_t nf = static_cast<size_t>(ctx.frameTracker().getSetup().getNumFramesInFlight());

    m_passes.resize(1);
    m_passes[0] = CreateColorPass(ctx, passReg, isReady);

    // m_passes[3] = ChunkOrchestrator{
    //     Pass{
    //         ComputeContributor{Distance{}},
    //     },
    //    ,
    //     &isReady};
}
} // namespace render_system::fog