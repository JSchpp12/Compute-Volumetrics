//#include "renderer/FinalizationRenderer.hpp"
//
//#include <starlight/command/command_order/GetPassInfo.hpp>
//#include <starlight/core/helper/queue/QueueHelpers.hpp>
//
//void renderer::FinalizationRenderer::addMemoryBarriersPost(vk::CommandBuffer cmdBuff,
//                                                           const star::common::FrameTracker &ft) const
//{
//}
//
//void renderer::FinalizationRenderer::addMemoryBarriersPre(vk::CommandBuffer cmdBuffer,
//                                                          const star::common::FrameTracker &ft) const
//{
//    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
//
//    std::vector<vk::ImageMemoryBarrier2> imgBarriers;
//    // assuming the voluem renderer will always run
//    if (ft.getCurrent().getNumTimesFrameProcessed() != 1)
//    {
//        imgBarriers.emplace_back(vk::ImageMemoryBarrier2().setImage(
//            m_vol->getRenderToImages()[ii]->getVulkanImage())
//            .setOldLayout(vk::ImageLayout::eGeneral)
//            .setSrcQueueFamilyIndex(m_computeQueueFamilyIndex)
//            .setDstQueueFamilyIndex(m_graphicsQueueFamilyIndex)
//            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
//            .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
//            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
//            .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
//            .setSubresourceRange(vk::ImageSubresourceRange().setAspectMask(vk::ImageAspectFlagBits::eColor).setBaseMipLevel(0).setLevelCount(1).setBaseArrayLayer(0).setLayerCount(1));
//    }
//}
//
//void renderer::FinalizationRenderer::recordCommands(vk::CommandBuffer &commandBuffer,
//                                                    const star::common::FrameTracker &ft, const uint64_t &frameIndex)
//{
//}
//
//void renderer::FinalizationRenderer::prepRender(star::common::IDeviceContext &c)
//{
//    auto &context = static_cast<star::core::device::DeviceContext &>(c);
//
//    m_computeQueueFamilyIndex =
//        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
//                                                  star::Queue_Type::Tcompute)
//            ->getParentQueueFamilyIndex();
//
//    m_graphicsQueueFamilyIndex =
//        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
//                                                  star::Queue_Type::Tgraphics)
//            ->getParentQueueFamilyIndex();
//    m_transferQueueFamilyIndex =
//        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
//                                                  star::Queue_Type::Ttransfer)
//            ->getParentQueueFamilyIndex();
//
//    this->star::core::renderer::HeadlessRenderer::prepRender(c);
//}
