#include "OffscreenRenderPhase.hpp"

#include "Allocator.hpp"

#include <starlight/core/renderer/RenderPhaseHelpers.hpp>

OffscreenRenderPhase::OffscreenRenderPhase(const star::core::CommandBus &cmdBus, vk::Device device)
    : m_device(device), m_cmdBus(&cmdBus)
{
}

void OffscreenRenderPhase::recordPreRenderPassCommands(vk::CommandBuffer &buffer, const star::common::FrameTracker &ft)
{
    const size_t index = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
    star::StarTextures::Texture *colorTex =
        m_renderingContext.recordDependentImage.get(m_renderTargets.colorHandles()[index]);
    star::StarTextures::Texture *depthTex =
        m_renderingContext.recordDependentImage.get(m_renderTargets.depthHandles()[index]);

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

        buffer.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(prepImages));
    }

    if (firstFramePassCounter > 0)
    {
        firstFramePassCounter--;
        if (firstFramePassCounter == 0)
            isFirstPass = false;
    }
}

void OffscreenRenderPhase::recordPostRenderingCalls(vk::CommandBuffer &buffer, const star::common::FrameTracker &ft)
{
    size_t index = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
    star::StarTextures::Texture *colorTex =
        m_renderingContext.recordDependentImage.get(m_renderTargets.colorHandles()[index]);
    star::StarTextures::Texture *depthTex =
        m_renderingContext.recordDependentImage.get(m_renderTargets.depthHandles()[index]);

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

void OffscreenRenderPhase::updateDependentData(star::core::device::DeviceContext &context)
{
    auto priorSync = star::core::renderer::GetNeighborConsumerSyncInfo(context.getCmdBus(), m_commandBuffer);

    auto result = m_frameData->frameUpdate(context, priorSync);
    auto &record = context.getManagerCommandBuffer().m_manager.get(m_commandBuffer);
    for (const auto &w : result.waits)
    {
        record.oneTimeWaitSemaphoreInfo.insert(w.handle, w.semaphore, w.waitStage, w.signalValue);
        m_renderingContext.addBufferToRenderingContext(context, w.handle);
    }
}

void OffscreenRenderPhase::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                               const star::common::FrameTracker &ft, const uint64_t &frameIndex)
{
    star::core::renderer::waitForTimelineSemaphore(*m_cmdBus, m_device, m_commandBuffer, ft);
    this->star::core::renderer::DefaultRenderPhase::recordCommandBuffer(commandBuffer, ft, frameIndex);
}

vk::RenderingAttachmentInfo OffscreenRenderPhase::prepareDynamicRenderingInfoColorAttachment(
    const star::common::FrameTracker &frameTracker)
{
    const auto tmp = this->DefaultRenderPhase::prepareDynamicRenderingInfoColorAttachment(frameTracker);

    const size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    return vk::RenderingAttachmentInfo()
        .setImageView(
            m_renderingContext.recordDependentImage.get(m_renderTargets.colorHandles()[index])->getImageView())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearValue().setColor({1.0f, 1.0f, 1.0f, 1.0f}));
}

std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> OffscreenRenderPhase::
    getSubmissionOverride()
{
    return star::core::renderer::makeEdgeAwareSubmissionOverride(m_cmdBus, &m_commandBuffer, false);
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
