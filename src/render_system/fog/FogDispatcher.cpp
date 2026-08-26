#include "render_system/fog/FogDispatcher.hpp"

#include "render_system/fog/commands/Distance.hpp"
#include "render_system/fog/commands/Pass.hpp"
#include "render_system/fog/commands/PostMemoryBarrierContributor.hpp"
#include "render_system/fog/commands/PreMemoryBarrierContributor.hpp"
#include "render_system/fog/commands/transmittance/TransmittancePrecompute.hpp"
#include "render_system/fog/policies/ShadowDepthAcquirePolicy.hpp"
#include "render_system/fog/policies/ShadowDepthReleaseBackPolicy.hpp"
#include "render_system/fog/struct/ShaderFlags.hpp"
#include "render_system/fog/struct/ShaderPushInfo.hpp"
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

static ChunkOrchestrator CreateTransmittancePrecomputePass(star::core::device::DeviceContext &ctx,
                                                           star::Handle &passReg, bool &isReady)
{
    const auto [graphicsQueueFamilyIndex, computeQueueFamilyIndex, transferQueueFamilyIndex] =
        GetQueueFamilyIndices(ctx);

    std::vector<commands::Pass> pass;
    pass.resize(3);

    const auto *queueInfo = ctx.getManagerCommandBuffer().m_manager.getInUseInfoForType(star::Queue_Type::Tcompute);
    assert(queueInfo != nullptr && "Failed to get queue info from manager");

    // Acquire the shadow depth (graphics -> compute) before the transmittance
    // pass -- it is the first shadow-depth consumer (rayInit reads the
    // non-compare sun depth at set 3). The color pass relies on this acquire.
    pass[0] = Pass{
        ComputeContributor{Init{ctx.getEngineResolution(), InitPassType::LightCamera}},
        PreMemoryBarrierContributor{transmittance::PreMemoryBarrierRecorder{transmittance::ShadowDepthAcquire{
            render_system::fog::makeShadowDepthAcquirePolicy(graphicsQueueFamilyIndex, computeQueueFamilyIndex)}}}};
    pass[1] = Pass{ComputeContributor{IndirectDispatch{}}};
    pass[2] = Pass{ComputeContributor{TransmittancePrecompute{}}};

    return ChunkOrchestrator{
        star::StarCommandBuffer(ctx.getDevice().getVulkanDevice(),
                                static_cast<int>(ctx.frameTracker().getSetup().getNumFramesInFlight()),
                                &queueInfo->pool, star::Queue_Type::Tcompute, false, false),
        std::move(pass), false, &isReady};
}

static ChunkOrchestrator CreateColorPass(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady)
{
    const auto [graphicsQueueFamilyIndex, computeQueueFamilyIndex, transferQueueFamilyIndex] =
        GetQueueFamilyIndices(ctx);

    std::vector<commands::Pass> pass;
    pass.resize(3);

    const auto *queueInfo = ctx.getManagerCommandBuffer().m_manager.getInUseInfoForType(star::Queue_Type::Tcompute);
    assert(queueInfo != nullptr && "Failed to get queue info from manager");

    QueueFamilyIndices info{
        .graphics = graphicsQueueFamilyIndex, .transfer = transferQueueFamilyIndex, .compute = computeQueueFamilyIndex};

    pass[0] = Pass{ComputeContributor{Init{ctx.getEngineResolution()}},
                   PreMemoryBarrierContributor{color::PreMemoryBarrierRecorder{color::PreDifferentFamilies{info}}}};

    pass[1] = Pass{ComputeContributor{IndirectDispatch{}}};

    pass[2] = Pass{ComputeContributor{Color{}},
                   PostMemoryBarrierContributor{color::PostMemoryBarrierRecorder{
                       color::PostDifferentFamilies{info, render_system::fog::makeShadowDepthReleaseBackPolicy(
                                                              graphicsQueueFamilyIndex, computeQueueFamilyIndex)}}}};

    return ChunkOrchestrator{
        star::StarCommandBuffer(ctx.getDevice().getVulkanDevice(),
                                static_cast<int>(ctx.frameTracker().getSetup().getNumFramesInFlight()),
                                &queueInfo->pool, star::Queue_Type::Tcompute, false, false),
        std::move(pass), false, &isReady};
}

static ChunkOrchestrator CreateDepthPass(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady)
{
    const auto [graphicsQueueFamilyIndex, computeQueueFamilyIndex, transferQueueFamilyIndex] =
        GetQueueFamilyIndices(ctx);

    std::vector<commands::Pass> pass;
    pass.resize(3);

    const auto *queueInfo = ctx.getManagerCommandBuffer().m_manager.getInUseInfoForType(star::Queue_Type::Tcompute);
    assert(queueInfo != nullptr && "Failed to get queue info from manager");

    QueueFamilyIndices info{
        .graphics = graphicsQueueFamilyIndex, .transfer = transferQueueFamilyIndex, .compute = computeQueueFamilyIndex};

    pass[0] =
        Pass{ComputeContributor{Init{ctx.getEngineResolution(), InitPassType::Camera, true}},
             PreMemoryBarrierContributor{distance::PreMemoryBarrierRecorder{distance::PreDifferentFamilies{info}}}};

    pass[1] = Pass{ComputeContributor{IndirectDispatch{}}};

    pass[2] =
        Pass{ComputeContributor{Distance{}},
             PostMemoryBarrierContributor{distance::PostMemoryBarrierRecorder{distance::PostDifferentFamilies{info}}}};

    return ChunkOrchestrator{
        star::StarCommandBuffer(ctx.getDevice().getVulkanDevice(),
                                static_cast<int>(ctx.frameTracker().getSetup().getNumFramesInFlight()),
                                &queueInfo->pool, star::Queue_Type::Tcompute, false, false),
        std::move(pass), true, &isReady};
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

    vk::Semaphore workingSemaphore{VK_NULL_HANDLE};
    {
        auto gCmd = star::command_order::GetPassInfo{registration};
        m_cmdBus->submit(gCmd);
        workingSemaphore = gCmd.getReply().get().signaledSemaphore;
    }

    for (uint8_t i{0}; i < m_numCbRecorded; i++)
        m_cbSubmitInfo[i] = vk::CommandBufferSubmitInfo().setCommandBuffer(m_passes[i].getCmdBuffer().buffer(ii));

    assert(dataSemaphores.size() == dataWaitPoints.size());

    uint32_t waitInfoCount{0};
    {
        auto wait = m_syncApproach.getWaitInfo();
        for (uint8_t i{0}; i < wait.count; i++)
        {
            m_cbSubmitWait[waitInfoCount] = wait.info[i];
            waitInfoCount++;
        }
    }

    for (size_t i{0}; i < dataWaitPoints.size(); i++)
    {
        m_cbSubmitWait[waitInfoCount] = vk::SemaphoreSubmitInfo()
                                            .setSemaphore(dataSemaphores[i])
                                            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
        waitInfoCount++;
    }

    vk::SemaphoreSubmitInfo signalInfo = vk::SemaphoreSubmitInfo()
                                             .setSemaphore(workingSemaphore)
                                             .setValue(getTimelineDoneSignalValue(ft))
                                             .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

    {
        vk::SubmitInfo2 submitInfo = vk::SubmitInfo2()
                                         .setPSignalSemaphoreInfos(&signalInfo)
                                         .setSignalSemaphoreInfoCount(1)
                                         .setPCommandBufferInfos(m_cbSubmitInfo.data())
                                         .setCommandBufferInfoCount(m_numCbRecorded)
                                         .setPWaitSemaphoreInfos(m_cbSubmitWait.data())
                                         .setWaitSemaphoreInfoCount(waitInfoCount);

        queue.getVulkanQueue().submit2(submitInfo);
    }
}

void FogDispatcher::recordCommands(DispatchInfo &dInfo, const star::common::FrameTracker &ft, const PassInfo &pInfo,
                                   const PassPipelineInfo &pipeInfo)
{
    assert(m_passes.size() > 0);
    m_numCbRecorded = 0;

    // TODO: move the wait for semaphore value from the volume renderer to here
    for (size_t i{0}; i < m_passes.size(); i++)
    {
        switch (i)
        {
        case 0: // transmittance precompute pass
            // rayInit gates against the sun (orthographic) shadow depth
            // (set 3 binds the non-compare sun depth) instead of the camera depth.
            dInfo.shaderOptionFlags |=
                Pack(InitShaderFlags::EnableAabbTest | InitShaderFlags::EnableShadowDepthTest, MarchShaderFlags::None);
            break;
        case 1: // color pass
            switch (pipeInfo.fogType)
            {
            case (Fog::Type::sExponential):
            case (Fog::Type::sLinear):
                dInfo.shaderOptionFlags |= Pack(InitShaderFlags::EnableColorOutput, MarchShaderFlags::None);
                break;
            default:
                dInfo.shaderOptionFlags |= Pack(InitShaderFlags::EnableDepthtest | InitShaderFlags::EnableAabbTest |
                                                    InitShaderFlags::EnableColorOutput,
                                                MarchShaderFlags::None);
            }
            break;
        case 2: // depth pass (marched only)
            if (pipeInfo.fogType == Fog::Type::sMarched)
                dInfo.shaderOptionFlags |= Pack(InitShaderFlags::EnableAabbTest, MarchShaderFlags::None);
            break;
        }

        // transmittance and color always dispatch; the distance compute only runs for the marched option.
        const bool dispatch = (i == 0) || (i == 1) || (i == 2 && pipeInfo.fogType == Fog::Type::sMarched);
        if (dispatch)
        {
            m_passes[i].recordCommands(dInfo, pInfo, pipeInfo, ft);
            m_numCbRecorded++;
        }
    }
}

uint64_t FogDispatcher::getTimelineDoneSignalValue(const star::common::FrameTracker &ft) const
{
    return m_syncApproach.getSignalInfo().value;
}

void FogDispatcher::createChunks(star::core::device::DeviceContext &ctx, star::Handle &passReg, bool &isReady)
{
    const size_t nf = static_cast<size_t>(ctx.frameTracker().getSetup().getNumFramesInFlight());

    m_passes.resize(3);
    m_passes[0] = CreateTransmittancePrecomputePass(ctx, passReg, isReady);
    m_passes[1] = CreateColorPass(ctx, passReg, isReady);
    m_passes[2] = CreateDepthPass(ctx, passReg, isReady);

    m_cbSubmitInfo.resize(3);
    m_numCbRecorded = 0;
}
} // namespace render_system::fog
