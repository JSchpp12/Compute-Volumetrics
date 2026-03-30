#include "renderer/FinalizationRenderer.hpp"

#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

void renderer::FinalizationRenderer::addMemoryBarriersPost(vk::CommandBuffer cmdBuff,
                                                           const star::common::FrameTracker &ft) const
{
}

void renderer::FinalizationRenderer::recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer,
                                                                 const star::common::FrameTracker &ft)
{
    addMemoryBarriersPre(commandBuffer, ft);

    this->star::core::renderer::HeadlessRenderer::recordPreRenderPassCommands(commandBuffer, ft);
}

void renderer::FinalizationRenderer::recordPostRenderingCalls(vk::CommandBuffer &commandBuffer,
                                                              const star::common::FrameTracker &ft)
{
    this->star::core::renderer::HeadlessRenderer::recordPostRenderingCalls(commandBuffer, ft);
}

void renderer::FinalizationRenderer::addMemoryBarriersPre(vk::CommandBuffer cmdBuffer,
                                                          const star::common::FrameTracker &ft) const
{
    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());

    // assuming the voluem renderer will always run
    if (ft.getCurrent().getNumTimesFrameProcessed() != 1)
    {
        const auto *cImage =
            m_renderingContext.recordDependentImage.get(m_renderToImages[ft.getCurrent().getFrameInFlightIndex()]);

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
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAllGraphics)
                .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite |
                                  vk::AccessFlagBits2::eColorAttachmentRead)
                .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)};

        // assuming for now that the transfer will always run in headless
        cmdBuffer.pipelineBarrier2(
            vk::DependencyInfo().setPImageMemoryBarriers(imgBarriers).setImageMemoryBarrierCount(1));
    }
}

void renderer::FinalizationRenderer::prepRender(star::common::IDeviceContext &c)
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
