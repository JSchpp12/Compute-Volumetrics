#include "OffscreenRenderPhase.hpp"

#include "Allocator.hpp"

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/core/renderer/EdgeSubmission.hpp>
#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

void OffscreenRenderPhase::recordPreRenderPassCommands(vk::CommandBuffer &buffer, const star::common::FrameTracker &ft)
{
    const size_t index = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
    star::StarTextures::Texture *colorTex = m_renderingContext.recordDependentImage.get(m_renderTargets.colorHandles()[index]);
    star::StarTextures::Texture *depthTex = m_renderingContext.recordDependentImage.get(m_renderTargets.depthHandles()[index]);

    // need to transition the image from general to color attachment
    // also get ownership back
    auto prepImages = std::vector<vk::ImageMemoryBarrier2>{
        vk::ImageMemoryBarrier2()
            .setImage(colorTex->getVulkanImage())
            .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
            .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setBaseArrayLayer(0)
                                     .setLevelCount(1)
                                     .setLayerCount(1)),
        vk::ImageMemoryBarrier2()
            .setImage(depthTex->getVulkanImage())
            .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                             vk::PipelineStageFlagBits2::eLateFragmentTests)
            .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                              vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                     .setBaseMipLevel(0)
                                     .setBaseArrayLayer(0)
                                     .setLevelCount(1)
                                     .setLayerCount(1))};

    if (!isFirstPass)
    {
        prepImages[0]
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex)
            .setDstQueueFamilyIndex(this->graphicsQueueFamilyIndex);
        prepImages[1]
            .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex)
            .setDstQueueFamilyIndex(this->graphicsQueueFamilyIndex);
    }
    else
    {
        prepImages[0]
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);

        prepImages[1]
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
    }

    buffer.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(prepImages));
}

void OffscreenRenderPhase::recordPostRenderingCalls(vk::CommandBuffer &buffer, const star::common::FrameTracker &ft)
{
    size_t index = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
    star::StarTextures::Texture *colorTex = m_renderingContext.recordDependentImage.get(m_renderTargets.colorHandles()[index]);
    star::StarTextures::Texture *depthTex = m_renderingContext.recordDependentImage.get(m_renderTargets.depthHandles()[index]);

    {
        std::array<const vk::ImageMemoryBarrier2, 2> toCompute{
            vk::ImageMemoryBarrier2()
                .setImage(colorTex->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setNewLayout(vk::ImageLayout::eGeneral)
                .setSrcQueueFamilyIndex(this->graphicsQueueFamilyIndex)
                .setDstQueueFamilyIndex(this->computeQueueFamilyIndex)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
                .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
                .setDstAccessMask(vk::AccessFlagBits2::eNone)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)),
            vk::ImageMemoryBarrier2()
                .setImage(depthTex->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSrcQueueFamilyIndex(this->graphicsQueueFamilyIndex)
                .setDstQueueFamilyIndex(this->computeQueueFamilyIndex)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eLateFragmentTests |
                                 vk::PipelineStageFlagBits2::eEarlyFragmentTests)
                .setSrcAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
                .setDstAccessMask(vk::AccessFlagBits2::eNone)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1))};

        const auto depInfo =
            vk::DependencyInfo().setPImageMemoryBarriers(&toCompute.front()).setImageMemoryBarrierCount(2);

        buffer.pipelineBarrier2(depInfo);
    }
}

static std::tuple<vk::Semaphore, uint64_t, uint64_t> GetVolumeRendererSemaphoreFromNeighbor(
    const star::core::CommandBus &cmdBus, const star::Handle &myRegistration) noexcept
{
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    uint64_t toSignalValue{0};
    uint64_t currentSignalValue{0};

    auto cmd = star::command_order::GetPassInfo{myRegistration};
    cmdBus.submit(cmd);
    const auto &r = cmd.getReply().get();

    if (r.edges != nullptr)
    {
        for (const auto edge : *r.edges)
        {
            if (edge.producer == myRegistration)
            {
                auto nCmd = star::command_order::GetPassInfo{edge.consumer};
                cmdBus.submit(nCmd);

                const auto &nr = nCmd.getReply().get();
                assert(nr.wasProcessedOnLastFrame != nullptr &&
                       "Neighbor last submission records was not provided by command_order service. This indicates a "
                       "bug in that service.");

                semaphore = nr.signaledSemaphore;
                currentSignalValue = nr.currentSignalValue;
                toSignalValue = nr.toSignalValue;

                break;
            }
        }
    }

    return std::make_tuple(semaphore, toSignalValue, currentSignalValue);
}

void OffscreenRenderPhase::updateDependentData(star::core::device::DeviceContext &context)
{
    if (!ownsRenderResourceControllers)
        return;

    star::core::graphics::SemaphoreInfo transferSyncWithComputeInfo{};
    {
        auto [semaphore, toSignalValue, currentSignalValue] =
            GetVolumeRendererSemaphoreFromNeighbor(context.getCmdBus(), m_commandBuffer);

        transferSyncWithComputeInfo.semaphore = std::move(semaphore);
        transferSyncWithComputeInfo.signalValue = std::move(currentSignalValue);
    }

    auto result = m_frameData->frameUpdate(context, transferSyncWithComputeInfo);
    auto &record = context.getManagerCommandBuffer().m_manager.get(m_commandBuffer);
    for (const auto &w : result.waits)
    {
        record.oneTimeWaitSemaphoreInfo.insert(w.handle, w.semaphore, w.waitStage, w.signalValue);
        m_renderingContext.addBufferToRenderingContext(context, w.handle);
    }
}

void OffscreenRenderPhase::waitForSemaphore(const star::common::FrameTracker &ft) const
{
    uint64_t signalValue{0};
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    {
        star::command_order::GetPassInfo get{m_commandBuffer};
        m_cmdBus->submit(get);
        signalValue = get.getReply().get().currentSignalValue;
        semaphore = get.getReply().get().signaledSemaphore;
    }

    const uint64_t frameCount = ft.getCurrent().getNumTimesFrameProcessed();
    if (frameCount == signalValue)
    {
        assert(m_device != VK_NULL_HANDLE);

        auto result =
            m_device.waitSemaphores(vk::SemaphoreWaitInfo().setValues(frameCount).setSemaphores(semaphore), UINT64_MAX);

        if (result != vk::Result::eSuccess)
            STAR_THROW("Failed to wait for timeline semaphores");
    }
}

void OffscreenRenderPhase::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                               const star::common::FrameTracker &ft, const uint64_t &frameIndex)
{
    waitForSemaphore(ft);
    this->star::core::renderer::DefaultRenderPhase::recordCommandBuffer(commandBuffer, ft, frameIndex);
}

vk::RenderingAttachmentInfo OffscreenRenderPhase::prepareDynamicRenderingInfoColorAttachment(
    const star::common::FrameTracker &frameTracker)
{
    const auto tmp = this->DefaultRenderPhase::prepareDynamicRenderingInfoColorAttachment(frameTracker);

    const size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    return vk::RenderingAttachmentInfo()
        .setImageView(m_renderingContext.recordDependentImage.get(m_renderTargets.colorHandles()[index])->getImageView())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearValue().setColor({1.0f, 1.0f, 1.0f, 1.0f}));
}

std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> OffscreenRenderPhase::
    getSubmissionOverride()
{
    star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride overrideFn = std::bind(
        &OffscreenRenderPhase::submitBuffer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
        std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7);
    return overrideFn;
}

vk::RenderingAttachmentInfo OffscreenRenderPhase::prepareDynamicRenderingInfoDepthAttachment(
    const star::common::FrameTracker &frameTracker)
{
    const size_t i = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
    return vk::RenderingAttachmentInfoKHR()
        .setImageView(m_renderingContext.recordDependentImage.get(m_renderTargets.depthHandles()[i])->getImageView())
        .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue{1.0f}));
}

vk::Semaphore OffscreenRenderPhase::submitBuffer(star::StarCommandBuffer &buffer,
                                                 const star::common::FrameTracker &frameTracker,
                                                 std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                                 std::vector<vk::Semaphore> dataSemaphores,
                                                 std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                                 std::vector<std::optional<uint64_t>> previousSignaledValues,
                                                 star::StarQueue &queue)
{
    assert(m_cmdBus != nullptr);

    return star::core::renderer::submitEdgeAwarePass(*m_cmdBus, m_commandBuffer, buffer, frameTracker,
                                                      previousCommandBufferSemaphores, dataSemaphores, dataWaitPoints,
                                                      previousSignaledValues, queue,
                                                      /*signalBinaryCompletion=*/false);
}
