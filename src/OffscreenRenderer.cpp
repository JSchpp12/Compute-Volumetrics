#include "OffscreenRenderer.hpp"

OffscreenRenderer::OffscreenRenderer(star::StarScene &scene) : star::SceneRenderer(scene)
{
}

void OffscreenRenderer::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    vk::Viewport viewport = this->prepareRenderingViewport();
    commandBuffer.setViewport(0, viewport);

    this->recordPreRenderingCalls(commandBuffer, frameInFlightIndex);

    {
        // dynamic rendering used...so dont need all that extra stuff
        vk::RenderingAttachmentInfo colorAttachmentInfo =
            prepareDynamicRenderingInfoColorAttachment(frameInFlightIndex);
        vk::RenderingAttachmentInfo depthAttachmentInfo =
            prepareDynamicRenderingInfoDepthAttachment(frameInFlightIndex);

        auto renderArea = vk::Rect2D{vk::Offset2D{}, *this->swapChainExtent};
        vk::RenderingInfoKHR renderInfo{};
        renderInfo.renderArea = renderArea;
        renderInfo.layerCount = 1;
        renderInfo.pDepthAttachment = &depthAttachmentInfo;
        renderInfo.pColorAttachments = &colorAttachmentInfo;
        renderInfo.colorAttachmentCount = 1;
        commandBuffer.beginRendering(renderInfo);
    }

    this->recordRenderingCalls(commandBuffer, frameInFlightIndex);

    // std::vector<vk::ImageMemoryBarrier2> memBar =
    // std::vector<vk::ImageMemoryBarrier2>{createMemoryBarrierPrepForDepthCopy(this->renderToDepthImages.at(frameInFlightIndex)->getImage())};
    // vk::DependencyInfo depInfo;
    // depInfo.sType = vk::StructureType::eDependencyInfo;
    // depInfo.imageMemoryBarrierCount = 1;
    // depInfo.pImageMemoryBarriers = memBar.data();
    // depInfo.sType = vk::StructureType::eDependencyInfo;

    // commandBuffer.pipelineBarrier2(depInfo);

    // vk::CopyImageToBufferInfo2 imgCpy;
    // imgCpy.sType = vk::StructureType::eCopyImageToBufferInfo2;
    // imgCpy.dstBuffer =
    // this->depthInfoStorageBuffers.at(frameInFlightIndex)->getVulkanBuffer();
    // imgCpy.srcImage =
    // this->renderToDepthImages.at(frameInFlightIndex)->getImage();
    // imgCpy.regionCount

    commandBuffer.copyBufferToImage2(imgCpy);

    commandBuffer.endRendering();
}

std::vector<std::unique_ptr<star::StarTexture>> OffscreenRenderer::createRenderToImages(star::StarDevice &device,
                                                                                        const int &numFramesInFlight)
{
    std::vector<std::unique_ptr<star::StarTexture>> newRenderToImages =
        std::vector<std::unique_ptr<star::StarTexture>>();

    auto settings = star::StarTexture::RawTextureCreateSettings{
        static_cast<int>(this->swapChainExtent->width),
        static_cast<int>(this->swapChainExtent->height),
        4,
        1,
        1,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
        this->getCurrentRenderToImageFormat(),
        {vk::Format::eR8G8B8A8Unorm},
        vk::ImageAspectFlagBits::eColor,
        VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
        VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        vk::ImageLayout::eColorAttachmentOptimal,
        false,
        false,
        {},
        1.0f,
        vk::Filter::eNearest,
        "OffscreenRenderColor"};

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.push_back(
            std::make_unique<star::StarTexture>(settings, device.getDevice(), device.getAllocator().get()));

        auto oneTimeSetup = device.beginSingleTimeCommands();
        newRenderToImages.back()->transitionLayout(oneTimeSetup, vk::ImageLayout::eColorAttachmentOptimal,
                                                   vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
                                                   vk::PipelineStageFlagBits::eTopOfPipe,
                                                   vk::PipelineStageFlagBits::eColorAttachmentOutput);
        device.endSingleTimeCommands(oneTimeSetup);
    }

    return newRenderToImages;
}

std::vector<std::unique_ptr<star::StarTexture>> OffscreenRenderer::createRenderToDepthImages(
    star::StarDevice &device, const int &numFramesInFlight)
{
    std::vector<std::unique_ptr<star::StarTexture>> newRenderToImages =
        std::vector<std::unique_ptr<star::StarTexture>>();

    auto settings = star::StarTexture::RawTextureCreateSettings{
        static_cast<int>(this->swapChainExtent->width),
        static_cast<int>(this->swapChainExtent->height),
        1,
        1,
        1,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eStorage,
        this->findDepthFormat(device),
        {},
        vk::ImageAspectFlagBits::eDepth,
        VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
        VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        vk::ImageLayout::eDepthAttachmentOptimal,
        false,
        true,
        {},
        1.0f,
        vk::Filter::eNearest,
        "OffscreenDepths"};

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.push_back(
            std::make_unique<star::StarTexture>(settings, device.getDevice(), device.getAllocator().get()));

        auto oneTimeSetup = device.beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back()->getImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
        barrier.subresourceRange.levelCount = 1;   // image is not an array
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        oneTimeSetup.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,         // which pipeline stages should
                                                                                    // occurr before barrier
                                     vk::PipelineStageFlagBits::eLateFragmentTests, // pipeline stage in
                                                                                    // which operations will
                                                                                    // wait on the barrier
                                     {}, {}, nullptr, barrier);

        device.endSingleTimeCommands(oneTimeSetup);
    }

    return newRenderToImages;
}

std::vector<std::shared_ptr<star::StarBuffer>> OffscreenRenderer::createDepthBufferContainers(star::StarDevice &device)
{
    return std::vector<std::shared_ptr<star::StarBuffer>>();
}

star::Command_Buffer_Order_Index OffscreenRenderer::getCommandBufferOrderIndex()
{
    return star::Command_Buffer_Order_Index::first;
}

star::Command_Buffer_Order OffscreenRenderer::getCommandBufferOrder()
{
    return star::Command_Buffer_Order::before_render_pass;
}

vk::PipelineStageFlags OffscreenRenderer::getWaitStages()
{
    // should be able to wait until the fragment shader where the image produced
    // from the compute shader will be used
    return vk::PipelineStageFlagBits::eFragmentShader;
}

bool OffscreenRenderer::getWillBeSubmittedEachFrame()
{
    return true;
}

bool OffscreenRenderer::getWillBeRecordedOnce()
{
    return false;
}

vk::Format OffscreenRenderer::getCurrentRenderToImageFormat()
{
    return vk::Format::eR8G8B8A8Snorm;
}

vk::ImageMemoryBarrier2 OffscreenRenderer::createMemoryBarrierPrepForDepthCopy(const vk::Image &depthImage)
{
    vk::ImageMemoryBarrier2 memBar;
    memBar.sType = vk::StructureType::eImageMemoryBarrier2;
    memBar.srcStageMask =
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
    memBar.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
    memBar.dstStageMask = vk::PipelineStageFlagBits2::eCopy;
    memBar.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead;
    memBar.image = depthImage;
    memBar.oldLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    memBar.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    memBar.subresourceRange.baseArrayLayer = 0;
    memBar.subresourceRange.baseMipLevel = 0;
    memBar.subresourceRange.layerCount = 1;
    memBar.subresourceRange.levelCount = 1;
    memBar.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

    // vk::MemoryBarrier2 memBar;
    // memBar.sType = vk::StructureType::eMemoryBarrier2;
    // memBar.srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
    // vk::PipelineStageFlagBits2::eLateFragmentTests; memBar.srcAccessMask =
    // vk::AccessFlagBits2::eDepthStencilAttachmentWrite; memBar.dstStageMask =
    // vk::PipelineStageFlagBits2::eTransfer; memBar.dstAccessMask =
    // vk::AccessFlagBits2::eTransferRead;

    return memBar;
}
