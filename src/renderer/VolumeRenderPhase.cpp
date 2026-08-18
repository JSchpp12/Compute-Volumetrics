#include "renderer/VolumeRenderPhase.hpp"

#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/core/Exceptions.hpp>

#include <cassert>
#include <cstdint>
#include <functional>
#include <optional>
#include <tuple>
#include <vector>

static std::optional<star::command_order::get_pass_info::GatheredPassInfo> GetTransferNeighborInfo(
    const star::core::CommandBus &cmdBus, const star::Handle &commandBuffer,
    const std::optional<star::Handle> &transferNeighborHandle)
{
    if (!transferNeighborHandle.has_value())
        return std::nullopt;

    std::optional<star::command_order::get_pass_info::GatheredPassInfo> transferNeighborInfo{std::nullopt};

    auto getCmd = star::command_order::GetPassInfo(commandBuffer);
    cmdBus.submit(getCmd);

    // command deps
    const star::command_order::get_pass_info::GatheredPassInfo &ele = getCmd.getReply().get();

    if (ele.edges != nullptr)
    {
        for (const auto &edge : *ele.edges)
        {
            if (edge.producer == commandBuffer && edge.consumer == *transferNeighborHandle)
            {
                auto nGetCmd = star::command_order::GetPassInfo(edge.consumer);
                cmdBus.submit(nGetCmd);

                transferNeighborInfo = nGetCmd.getReply().get();

                break;
            }
        }
    }

    return transferNeighborInfo;
}

VolumeRenderPhase::VolumeRenderPhase(bool enableCutoffHighlighting)
    : m_distanceComputer(), m_chunkHandler(enableCutoffHighlighting)
{
}

void VolumeRenderPhase::frameUpdate(star::common::IDeviceContext &context)
{
    auto &c = static_cast<star::core::device::DeviceContext &>(context);
    const uint8_t frameInFlightIndex = c.frameTracker().getCurrent().getFrameInFlightIndex();

    updateRenderingContext(c, frameInFlightIndex);

    if (isRenderReady(c))
    {
        updateDependentData(c, frameInFlightIndex);
        gatherDependentExternalDataOrderingInfo(c, frameInFlightIndex);

        m_distanceComputer.frameUpdate(c);
    }
}

bool VolumeRenderPhase::isRenderReady(star::core::device::DeviceContext &context)
{
    if (isReady)
    {
        return true;
    }

    if (context.getPipelineManager().get(marchedPipeline)->isReady() &&
        context.getPipelineManager().get(linearPipeline)->isReady() &&
        context.getPipelineManager().get(expPipeline)->isReady() &&
        context.getPipelineManager().get(nanoVDBPipeline_hitBoundingBox)->isReady() &&
        context.getPipelineManager().get(nanoVDBPipeline_surface)->isReady() &&
        context.getPipelineManager().get(marchedHomogenousPipeline)->isReady() && m_distanceComputer.isReady(context) &&
        context.getPipelineManager().get(m_indirectDispatchPipe)->isReady() &&
        context.getPipelineManager().get(m_initPipe)->isReady())
    {
        isReady = true;
    }

    return isReady;
}

void VolumeRenderPhase::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                            const star::common::FrameTracker &frameTracker, const uint64_t &frameIndex)
{
    recordCommands(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()), frameTracker, frameIndex);
}

void VolumeRenderPhase::recordCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &ft,
                                       const uint64_t &frameIndex)
{
    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());

    auto tNeighbor = GetTransferNeighborInfo(*m_cmdBus, m_commandBuffer, m_transferNeighborHandle);

    render_system::fog::PassInfo tInfo{
        .globalCameraBuffer =
            m_frameData->getController(m_cameraRole)
                    ->willBeUpdatedThisFrame(ft.getCurrent().getGlobalFrameCounter(), ft)
                ? std::make_optional(m_renderingContext.bufferTransferRecords.get(
                      m_frameData->getController(m_cameraRole)->getHandle(ft.getCurrent().getFrameInFlightIndex())))
                : std::nullopt,
        .fogControllerBuffer = m_fogController->willBeUpdatedThisFrame(ft.getCurrent().getGlobalFrameCounter(), ft)
                                   ? std::make_optional(m_renderingContext.bufferTransferRecords.get(
                                         m_fogController->getHandle(ft.getCurrent().getFrameInFlightIndex())))
                                   : std::nullopt,
        .terrainPassInfo =
            {.renderToColor =
                 m_renderingContext.recordDependentImage
                     .get(m_offscreenPhase->getRenderTargets().colorHandles()[ft.getCurrent().getFrameInFlightIndex()])
                     ->getVulkanImage(),
             .renderToDepth =
                 m_renderingContext.recordDependentImage
                     .get(m_offscreenPhase->getRenderTargets().depthHandles()[ft.getCurrent().getFrameInFlightIndex()])
                     ->getVulkanImage()},
        .computeWriteToImage = computeWriteToImages[ft.getCurrent().getFrameInFlightIndex()]->getVulkanImage(),
        .computeRayAtCutoffDistance =
            computeRayAtCutoffDistanceBuffers[ft.getCurrent().getFrameInFlightIndex()].getVulkanBuffer(),
        .computeRayDistance = computeRayDistanceBuffers[ft.getCurrent().getFrameInFlightIndex()].getVulkanBuffer(),
        .transferWasRunLast = tNeighbor.has_value() ? tNeighbor.value().wasProcessedOnLastFrame->at(
                                                          ft.getCurrent().getFrameInFlightIndex())
                                                    : false,
        .transferWillBeRunThisFrame = tNeighbor.has_value() ? tNeighbor.value().isTriggeredThisFrame : false};

    m_pipeInfo.distancePipe = {.layout = m_distanceComputer.getLayout(), .pipeline = m_distanceComputer.getPipeline()};
    m_pipeInfo.staticShaderInfo = m_staticShaderInfo.get();
    m_pipeInfo.colorOnlyShaderInfo = m_dynamicShaderInfo.get();
    m_pipeInfo.distanceOnlyShaderInfo = m_distanceComputer.getDynamicShaderInfo();
    m_pipeInfo.indirectDispatchBuffer = m_activeRayStorage[ii]->getVulkanBuffer();
    m_pipeInfo.colorPipe.layout = *this->computePipelineLayout;
    m_pipeInfo.colorPipe.pipeline = this->m_renderingContext.pipeline->getVulkanPipeline();
    m_pipeInfo.fogType = this->currentFogType;

    vk::Semaphore workingSemaphore{VK_NULL_HANDLE};
    {
        uint64_t previousSignaledValue{0};
        auto gCmd = star::command_order::GetPassInfo{m_commandBuffer};
        m_cmdBus->submit(gCmd);
        const auto &r = gCmd.getReply().get();
        workingSemaphore = r.signaledSemaphore;
        previousSignaledValue = r.currentSignalValue;

        auto wait = vk::SemaphoreWaitInfo()
                        .setSemaphoreCount(1)
                        .setPSemaphores(&workingSemaphore)
                        .setPValues(&previousSignaledValue)
                        .setSemaphoreCount(1);

        try
        {
            const auto waitResult = m_device.waitSemaphores(wait, UINT64_MAX);
        }
        catch (const std::runtime_error &e)
        {
            STAR_THROW_CAUSE("Failed to wait for seamphores", e);
        }
    }

    render_system::fog::DispatchInfo dInfo{m_activeRayStorage[ii]->getVulkanBuffer()};

    m_chunkHandler.recordCommands(dInfo, ft, tInfo, m_pipeInfo);
}

uint64_t VolumeRenderPhase::getTimelineSignalValue(const star::common::FrameTracker &ft) const
{
    return m_chunkHandler.getTimelineDoneSignalValue(ft);
}

std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> VolumeRenderPhase::
    getSubmissionOverride()
{
    star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride overrideFn = std::bind(
        &VolumeRenderPhase::submitBuffer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
        std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7);
    return overrideFn;
}

vk::Semaphore VolumeRenderPhase::submitBuffer(star::StarCommandBuffer &buffer,
                                              const star::common::FrameTracker &frameTracker,
                                              std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                              std::vector<vk::Semaphore> &dataSemaphores,
                                              std::vector<vk::PipelineStageFlags> &dataWaitPoints,
                                              std::vector<std::optional<uint64_t>> &previousSignaledValues,
                                              star::StarQueue &queue)
{
    m_chunkHandler.submit(frameTracker, std::move(dataSemaphores), std::move(dataWaitPoints),
                          std::move(previousSignaledValues), queue, m_commandBuffer);

    return vk::Semaphore();
}

void VolumeRenderPhase::cleanupRender(star::common::IDeviceContext &context)
{
    auto &c = static_cast<star::core::device::DeviceContext &>(context);

    m_chunkHandler.cleanupRender(c);
    m_distanceComputer.cleanupRender(c);
    m_staticShaderInfo->cleanupRender(c.getDevice());
    m_dynamicShaderInfo->cleanupRender(c.getDevice());

    for (auto &image : computeWriteToImages)
    {
        image->cleanupRender(c.getDevice().getVulkanDevice());
    }
    for (auto &buffer : m_activeRayStorage)
    {
        buffer->cleanupRender(c.getDevice().getVulkanDevice());
    }
    for (auto &buffer : computeRayDistanceBuffers)
    {
        buffer.cleanupRender(c.getDevice().getVulkanDevice());
    }
    for (auto &buffer : computeRayAtCutoffDistanceBuffers)
    {
        buffer.cleanupRender(c.getDevice().getVulkanDevice());
    }

    c.getDevice().getVulkanDevice().destroyPipelineLayout(*this->computePipelineLayout);
}

std::vector<std::pair<vk::DescriptorType, const uint32_t>> VolumeRenderPhase::getDescriptorRequests(
    const int &numFramesInFlight)
{
    return std::vector<std::pair<vk::DescriptorType, const uint32_t>>{
        std::make_pair(vk::DescriptorType::eStorageImage, 1 + (3 * numFramesInFlight * 50)),
        std::make_pair(vk::DescriptorType::eUniformBuffer, 1 + (4 * numFramesInFlight * 50)),
        std::make_pair(vk::DescriptorType::eStorageBuffer, 6 * numFramesInFlight),
        std::make_pair(vk::DescriptorType::eCombinedImageSampler, 805 * numFramesInFlight)};
}

void VolumeRenderPhase::recordDependentDataPipelineBarriers(vk::CommandBuffer &commandBuffer,
                                                            const uint8_t &frameinFlightIndex,
                                                            const uint64_t &frameIndex)
{
}

void VolumeRenderPhase::gatherDependentExternalDataOrderingInfo(star::core::device::DeviceContext &context,
                                                                const uint8_t &frameInFlightIndex)
{
}

void VolumeRenderPhase::updateDependentData(star::core::device::DeviceContext &context,
                                            const uint8_t &frameInFlightIndex)
{
    {
        std::optional<star::core::graphics::SemaphoreInfo> transferWaitOnLastCompute{std::nullopt};
        {
            auto cmd = star::command_order::GetPassInfo{m_commandBuffer};
            context.getCmdBus().submit(cmd);
            const auto &reply = cmd.getReply().get();
            if (reply.signaledSemaphore)
            {
                transferWaitOnLastCompute = star::core::graphics::SemaphoreInfo{.signalValue = reply.currentSignalValue,
                                                                                .semaphore = reply.signaledSemaphore};
            }
        }

        auto fogResult = m_volumeFrameData->frameUpdate(context, transferWaitOnLastCompute);
        auto &record = context.getManagerCommandBuffer().m_manager.get(m_commandBuffer);
        for (const auto &w : fogResult.waits)
        {
            record.oneTimeWaitSemaphoreInfo.insert(w.handle, w.semaphore, w.waitStage, w.signalValue);
            m_renderingContext.addBufferToRenderingContext(context, w.handle);
        }
    }

    if (m_frameData->getController(m_cameraRole)
            ->willBeUpdatedThisFrame(context.frameTracker().getCurrent().getGlobalFrameCounter(),
                                     context.frameTracker()))
    {
        m_renderingContext.addBufferToRenderingContext(
            context, m_frameData->getController(m_cameraRole)->getHandle(frameInFlightIndex));
    }
}

void VolumeRenderPhase::updateRenderingContext(star::core::device::DeviceContext &context,
                                               const uint8_t &frameInFlightIndex)
{
    switch (this->currentFogType)
    {
    case (Fog::Type::sMarched):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->marchedPipeline)->builtPipeline;
        break;
    case (Fog::Type::sLinear):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->linearPipeline)->builtPipeline;
        break;
    case (Fog::Type::sExponential):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->expPipeline)->builtPipeline;
        break;
    case (Fog::Type::sMarchedHomogenous):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->marchedHomogenousPipeline)->builtPipeline;
        break;
    case (Fog::Type::sNanoBoundingBox):
        m_renderingContext.pipeline =
            &context.getPipelineManager().get(this->nanoVDBPipeline_hitBoundingBox)->builtPipeline;
        break;
    case (Fog::Type::sNanoSurface):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->nanoVDBPipeline_surface)->builtPipeline;
        break;
    default:
        throw std::runtime_error("Unsupported type");
    }
}

std::vector<star::StarBuffers::Buffer> VolumeRenderPhase::createComputeWriteToBuffers(
    star::core::device::DeviceContext &context, const vk::Extent2D &screenSize, const size_t &dataTypeSize,
    const std::string &debugName, const size_t &numToCreate)
{
    auto buffers = std::vector<star::StarBuffers::Buffer>(numToCreate);

    const vk::DeviceSize bufferSize = screenSize.width * screenSize.height * dataTypeSize;
    auto builder =
        star::StarBuffers::Buffer::Builder(context.getDevice().getAllocator().get())
            .setAllocationCreateInfo(
                star::Allocator::AllocationBuilder()
                    .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                    .setUsage(VMA_MEMORY_USAGE_AUTO)
                    .build(),
                vk::BufferCreateInfo()
                    .setSharingMode(vk::SharingMode::eExclusive)
                    .setSize(bufferSize)
                    .setUsage(vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer |
                              vk::BufferUsageFlagBits::eTransferDst),
                debugName)
            .setInstanceCount(1)
            .setInstanceSize(bufferSize);

    for (size_t i{0}; i < numToCreate; i++)
    {
        buffers[i] = builder.build();
    }

    return buffers;
}
