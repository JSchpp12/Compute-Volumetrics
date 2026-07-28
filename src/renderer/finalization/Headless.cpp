#include "renderer/finalization/Headless.hpp"

#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

void renderer::finalization::Headless::addMemoryBarriersPost(vk::CommandBuffer cmdBuff,
                                                             const star::common::FrameTracker &ft) const
{
}

void renderer::finalization::Headless::recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer,
                                                                   const star::common::FrameTracker &ft)
{
    addMemoryBarriersPre(commandBuffer, ft);

    this->star::core::renderer::HeadlessRenderer::recordPreRenderPassCommands(commandBuffer, ft);
}

void renderer::finalization::Headless::recordPostRenderingCalls(vk::CommandBuffer &commandBuffer,
                                                                const star::common::FrameTracker &ft)
{
    addMemoryBarriersPost(commandBuffer, ft);

    this->star::core::renderer::HeadlessRenderer::recordPostRenderingCalls(commandBuffer, ft);
}

void renderer::finalization::Headless::addMemoryBarriersPre(vk::CommandBuffer cmdBuffer,
                                                            const star::common::FrameTracker &ft) const
{
    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());

    auto *cImage =
        m_renderingContext.recordDependentImage.get(m_renderToImages[ft.getCurrent().getFrameInFlightIndex()]);

    // assuming the voluem renderer will always run
    if (ft.getCurrent().getNumTimesFrameProcessed() != 0)
    {
        assert(cImage != nullptr && "Render to color images needed to be added to rendering context");
        vk::ImageMemoryBarrier2 imgBarriers[1]{
            vk::ImageMemoryBarrier2()
                .setImage(cImage->getVulkanImage())
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1))
                .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAllGraphics)
                .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite |
                                  vk::AccessFlagBits2::eColorAttachmentRead)
                .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)};
        cmdBuffer.pipelineBarrier2(
            vk::DependencyInfo().setPImageMemoryBarriers(imgBarriers).setImageMemoryBarrierCount(1));
        cImage->setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
    }
    else
    {
        auto *dImage =
            m_renderingContext.recordDependentImage.get(m_renderToDepthImages[ft.getCurrent().getFrameInFlightIndex()]);
        assert(dImage != nullptr && cImage != nullptr);
        vk::ImageMemoryBarrier2 imgBarriers[2]{
            vk::ImageMemoryBarrier2()
                .setImage(cImage->getVulkanImage())
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1))
                .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAllGraphics)
                .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite |
                                  vk::AccessFlagBits2::eColorAttachmentRead)
                .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal),
            vk::ImageMemoryBarrier2()
                .setImage(dImage->getVulkanImage())
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1))
                .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits2::eLateFragmentTests)
                .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
                .setOldLayout(vk::ImageLayout::eUndefined)
                .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)};

        cImage->setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
        dImage->setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
        cmdBuffer.pipelineBarrier2(
            vk::DependencyInfo().setPImageMemoryBarriers(imgBarriers).setImageMemoryBarrierCount(2));
    }
}

void renderer::finalization::Headless::prepRender(star::common::IDeviceContext &c)
{
    auto &context = static_cast<star::core::device::DeviceContext &>(c);

    m_computeQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tcompute)
            ->getParentQueueFamilyIndex();

    m_graphicsQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();
    m_transferQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Ttransfer)
            ->getParentQueueFamilyIndex();

    this->star::core::renderer::HeadlessRenderer::prepRender(c);
}
