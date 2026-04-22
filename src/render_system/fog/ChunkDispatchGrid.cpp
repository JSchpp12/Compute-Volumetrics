#include "render_system/fog/ChunkDispatchGrid.hpp"

#include "render_system/fog/commands/Distance.hpp"
#include "render_system/fog/commands/FullPass.hpp"
#include "render_system/fog/commands/PostMemoryBarrierDifferentFamilies.hpp"
#include "render_system/fog/commands/PreMemoryBarrierDifferentFamilies.hpp"

#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

using namespace render_system::fog;
using namespace render_system::fog::commands;
using namespace render_system::fog::sync;

static std::array<uint32_t, 2> CalculateWorkgroupSize(const vk::Extent2D &screensize)
{
    const uint32_t width = static_cast<uint32_t>(std::ceil(static_cast<float>(screensize.width) / 16.0f));
    const uint32_t height = static_cast<uint32_t>(std::ceil(static_cast<float>(screensize.height) / 16.0f));

    return {width, height};
}

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

void render_system::fog::ChunkDispatchGrid::createChunkHandlers(star::core::device::DeviceContext &ctx,
                                                                star::Handle &passRegistration, bool &isReady)
{
    const size_t nf = static_cast<size_t>(ctx.frameTracker().getSetup().getNumFramesInFlight());
    const size_t total = numChunks();
    m_chunkHandlers.resize(total);

    const auto [graphicsQueueFamilyIndex, computeQueueFamilyIndex, transferQueueFamilyIndex] =
        GetQueueFamilyIndices(ctx);

    const auto *queueInfo = ctx.getManagerCommandBuffer().m_manager.getInUseInfoForType(star::Queue_Type::Tcompute);
    assert(queueInfo != nullptr && "Failed to get queue info from manager");
    assert(total % 2 == 0 && "chunks must be an even size for now");

    m_chunkHandlers[0] = render_system::fog::ChunkOrchestrator{
        star::StarCommandBuffer{ctx.getDevice().getVulkanDevice(), static_cast<int>(nf), &queueInfo->pool,
                                star::Queue_Type::Tcompute, false, false},
        FullPass{ComputeContributor{Color{}},
                 PreMemoryBarrierContributor{PreMemoryBarrierDifferentFamilies{
                     computeQueueFamilyIndex, graphicsQueueFamilyIndex, transferQueueFamilyIndex}}},
        SyncProvider{signal::CalcFromFt(0, total, &ctx.frameTracker()),
                     wait::GatherFromCO{passRegistration, &ctx.getCmdBus()}},
        &isReady};

    for (int i{1}; i < total - 1; i++)
    {
        FullPass fp = i % 2 == 0 ? FullPass{ComputeContributor{Color{}}} : FullPass{ComputeContributor{Distance{}}};

        m_chunkHandlers[i] =
            ChunkOrchestrator{star::StarCommandBuffer{ctx.getDevice().getVulkanDevice(), static_cast<int>(nf),
                                                      &queueInfo->pool, star::Queue_Type::Tcompute, false, false},
                              std::move(fp),
                              SyncProvider{signal::CalcFromFt(i, total, &ctx.frameTracker()),
                                           wait::WaitForFirstChunk(total, &ctx.frameTracker())},
                              &isReady};
    }

    m_chunkHandlers[total - 1] = render_system::fog::ChunkOrchestrator{
        star::StarCommandBuffer{ctx.getDevice().getVulkanDevice(), static_cast<int>(nf), &queueInfo->pool,
                                star::Queue_Type::Tcompute, false, false},
        FullPass{ComputeContributor{Distance{}},
                 PostMemoryBarrierContributor{PostMemoryBarrierDifferentFamilies{
                     computeQueueFamilyIndex, graphicsQueueFamilyIndex, transferQueueFamilyIndex}}},
        SyncProvider{signal::CalcFromFt(total - 1, total, &ctx.frameTracker()),
                     wait::WaitForFirstChunk(total, &ctx.frameTracker())},
        &isReady};
}

void render_system::fog::ChunkDispatchGrid::prepRender(star::core::device::DeviceContext &ctx,
                                                       star::Handle &passRegistration, bool &isReady)
{
    m_cmdBus = &ctx.getCmdBus();

    // create the actual chunk handlers
    createChunkHandlers(ctx, passRegistration, isReady);

    m_workgroupSize = CalculateWorkgroupSize(ctx.getEngineResolution());
}

void render_system::fog::ChunkDispatchGrid::cleanupRender(star::core::device::DeviceContext &ctx)
{
    for (size_t i{0}; i < m_chunkHandlers.size(); i++)
    {
        m_chunkHandlers[i].cleanupRender(ctx);
    }
}

void render_system::fog::ChunkDispatchGrid::recordAllChunks(const star::common::FrameTracker &ft, const PassInfo &pInfo,
                                                            const PassPipelineInfo &pipeInfo, Fog::Type type)
{
    const uint32_t wx = m_workgroupSize[0];
    const uint32_t wy = m_workgroupSize[1];

    const uint32_t nx = static_cast<uint32_t>(m_numChunksPerDimension[0]);
    const uint32_t ny = static_cast<uint32_t>(m_numChunksPerDimension[1]);

    const uint32_t patchX = (wx + nx - 1) / nx;
    const uint32_t patchY = (wy + ny - 1) / ny;

    uint32_t halfX = wx / 2;
    uint32_t halfY = wy / 2;
    DispatchInfo dInfo{.workgroupSize = {patchX, patchY}, .localThreadGroupSize = {16, 16}};

    const size_t patches = numPatches();
    // total num passes
    for (size_t i{0}; i < patches; i++)
    {
        const uint32_t col = static_cast<uint32_t>(i) % nx;
        const uint32_t row = static_cast<uint32_t>(i) / nx;

        dInfo.chunkOffset[0] = col * patchX;
        dInfo.chunkOffset[1] = row * patchY;
        dInfo.chunkOffsetPixels[0] = dInfo.chunkOffset[0] * dInfo.localThreadGroupSize[0];
        dInfo.chunkOffsetPixels[1] = dInfo.chunkOffset[1] * dInfo.localThreadGroupSize[1];

         const size_t baseI = i * 2;
        // cmds per pass
        for (size_t j{baseI}; j < (baseI) + 2; j++)
        {
            m_chunkHandlers[j].recordCommands(dInfo, pInfo, pipeInfo, ft, type);
        }
    }
}

void render_system::fog::ChunkDispatchGrid::submitAllChunks(const star::common::FrameTracker &ft,
                                                            std::vector<vk::Semaphore> dataSemaphores,
                                                            std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                                            std::vector<std::optional<uint64_t>> previousSignaledValues,
                                                            star::StarQueue &queue, const star::Handle &registration)
{
    const size_t totalChunks = m_chunkHandlers.size();
    if (totalChunks == 0)
        return;

    size_t kChunksPerSubmission = 2;
    const size_t numSubmissions = (totalChunks + kChunksPerSubmission - 1) / kChunksPerSubmission;

    std::vector<vk::CommandBufferSubmitInfo> chunkCbInfo(totalChunks);
    WaitInfo firstSubmissionWaitInfo{};
    vk::SemaphoreSubmitInfo lastSubmissionSignalInfo{};

    {
        auto gCmd = star::command_order::GetPassInfo{registration};
        m_cmdBus->submit(gCmd);
        const auto workingSemaphore = gCmd.getReply().get().signaledSemaphore;

        for (size_t i{0}; i < totalChunks; i++)
        {
            chunkCbInfo[i] = m_chunkHandlers[i].getSubmitInfo(ft);
        }

        firstSubmissionWaitInfo = m_chunkHandlers.front().getWaitInfo(ft);
        lastSubmissionSignalInfo = m_chunkHandlers.back().getSignalInfo(ft);
        lastSubmissionSignalInfo.setSemaphore(workingSemaphore);
    }

    assert(dataSemaphores.size() == dataWaitPoints.size());
    assert(dataWaitPoints.size() + firstSubmissionWaitInfo.count < 5 &&
           "Current implementation of wait info container only allows 5");

    for (size_t i{0}; i < dataWaitPoints.size(); i++)
    {
        auto waitInfo = vk::SemaphoreSubmitInfo()
                            .setSemaphore(dataSemaphores[i])
                            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

        if (i < previousSignaledValues.size() && previousSignaledValues[i].has_value())
            waitInfo.setValue(previousSignaledValues[i].value());

        auto &w = firstSubmissionWaitInfo;
        w.info[w.count] = waitInfo;
        w.count++;
    }

    std::vector<std::vector<vk::CommandBufferSubmitInfo>> batchedCbInfo(numSubmissions);
    std::vector<std::array<vk::SemaphoreSubmitInfo, 5>> batchedWaitInfo(numSubmissions);
    std::vector<vk::SubmitInfo2> submitInfo(numSubmissions);

    for (size_t submissionIndex{0}; submissionIndex < numSubmissions; submissionIndex++)
    {
        const size_t beginChunk = submissionIndex * kChunksPerSubmission;
        size_t endChunk = beginChunk + kChunksPerSubmission;
        if (endChunk > totalChunks)
            endChunk = totalChunks;

        auto &cbBatch = batchedCbInfo[submissionIndex];
        for (size_t chunkIndex{beginChunk}; chunkIndex < endChunk; chunkIndex++)
        {
            cbBatch.push_back(chunkCbInfo[chunkIndex]);
        }

        auto &submit = submitInfo[submissionIndex];
        submit.setPCommandBufferInfos(cbBatch.data()).setCommandBufferInfoCount(static_cast<uint32_t>(cbBatch.size()));

        if (submissionIndex == 0)
        {
            for (size_t waitIndex{0}; waitIndex < firstSubmissionWaitInfo.count; waitIndex++)
            {
                batchedWaitInfo[submissionIndex][waitIndex] = firstSubmissionWaitInfo.info[waitIndex];
            }

            submit.setPWaitSemaphoreInfos(batchedWaitInfo[submissionIndex].data())
                .setWaitSemaphoreInfoCount(firstSubmissionWaitInfo.count);
        }

        if (submissionIndex == numSubmissions - 1)
        {
            submit.setPSignalSemaphoreInfos(&lastSubmissionSignalInfo).setSignalSemaphoreInfoCount(1);
        }
    }

    queue.getVulkanQueue().submit2(submitInfo);
}

uint64_t render_system::fog::ChunkDispatchGrid::getTimelineDoneSignalValue(const star::common::FrameTracker &ft) const
{
    // get the last chunk since this will signify to others that this is the value to wait for
    return m_chunkHandlers.back().getSignalInfo(ft).value;
}
