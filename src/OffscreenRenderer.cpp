#include "OffscreenRenderer.hpp"

#include "Allocator.hpp"

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

    commandBuffer.endRendering();
}

std::vector<std::unique_ptr<star::StarTexture>> OffscreenRenderer::createRenderToImages(star::StarDevice &device,
                                                                                        const int &numFramesInFlight)
{
    std::vector<std::unique_ptr<star::StarTexture>> newRenderToImages =
        std::vector<std::unique_ptr<star::StarTexture>>();

    std::vector<uint32_t> indices = std::vector<uint32_t>();
    indices.push_back(device.getQueueFamily(star::Queue_Type::Tgraphics).getQueueFamilyIndex());
    if (device.getQueueFamily(star::Queue_Type::Tpresent).getQueueFamilyIndex() != indices.back())
    {
        indices.push_back(device.getQueueFamily(star::Queue_Type::Tpresent).getQueueFamilyIndex());
    }

    auto builder =
        star::StarTexture::Builder(device.getDevice(), device.getAllocator().get())
            .setCreateInfo(
                star::Allocator::AllocationBuilder()
                    .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                    .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .build(),
                vk::ImageCreateInfo()
                    .setExtent(vk::Extent3D()
                                   .setWidth(static_cast<int>(this->swapChainExtent->width))
                                   .setHeight(static_cast<int>(this->swapChainExtent->height))
                                   .setDepth(1))
                    .setFlags(vk::ImageCreateFlagBits::eMutableFormat)
                    .setSharingMode(indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent)
                    .setQueueFamilyIndexCount(indices.size())
                    .setPQueueFamilyIndices(indices.data())
                    .setArrayLayers(1)
                    .setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage)
                    .setImageType(vk::ImageType::e2D)
                    .setMipLevels(1)
                    .setTiling(vk::ImageTiling::eOptimal)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setSamples(vk::SampleCountFlagBits::e1),
                "OffscreenRenderToImages")
            .setBaseFormat(this->getCurrentRenderToImageFormat())
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e2D)
                             .setFormat(this->getCurrentRenderToImageFormat())
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)))
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e2D)
                             .setFormat(vk::Format::eR8G8B8A8Unorm)
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)));

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());

        // auto oneTimeSetup = device.beginSingleTimeCommands();

        // star::StarTexture::TransitionImageLayout(*newRenderToImages.back(), oneTimeSetup,
        // vk::ImageLayout::eColorAttachmentOptimal,
        //                                            vk::AccessFlagBits::eNone,
        //                                            vk::AccessFlagBits::eColorAttachmentWrite,
        //                                            vk::PipelineStageFlagBits::eTopOfPipe,
        //                                            vk::PipelineStageFlagBits::eColorAttachmentOutput);

        // device.endSingleTimeCommands(oneTimeSetup);

        auto oneTimeSetup = device.beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back()->getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
        barrier.subresourceRange.levelCount = 1;   // image is not an array
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        oneTimeSetup.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,             // which pipeline stages should
                                                                                        // occurr before barrier
                                     vk::PipelineStageFlagBits::eColorAttachmentOutput, // pipeline stage in
                                                                                        // which operations will
                                                                                        // wait on the barrier
                                     {}, {}, nullptr, barrier);

        device.endSingleTimeCommands(oneTimeSetup);
    }

    return newRenderToImages;
}

std::vector<std::unique_ptr<star::StarTexture>> OffscreenRenderer::createRenderToDepthImages(
    star::StarDevice &device, const int &numFramesInFlight)
{
    std::vector<std::unique_ptr<star::StarTexture>> newRenderToImages =
        std::vector<std::unique_ptr<star::StarTexture>>();

    std::vector<uint32_t> indices = std::vector<uint32_t>();
    indices.push_back(device.getQueueFamily(star::Queue_Type::Tgraphics).getQueueFamilyIndex());
    if (device.getQueueFamily(star::Queue_Type::Tpresent).getQueueFamilyIndex() != indices.back())
    {
        indices.push_back(device.getQueueFamily(star::Queue_Type::Tpresent).getQueueFamilyIndex());
    }

    auto builder =
        star::StarTexture::Builder(device.getDevice(), device.getAllocator().get())
            .setCreateInfo(
                star::Allocator::AllocationBuilder()
                    .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                    .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .build(),
                vk::ImageCreateInfo()
                    .setExtent(vk::Extent3D()
                                   .setWidth(static_cast<int>(this->swapChainExtent->width))
                                   .setHeight(static_cast<int>(this->swapChainExtent->height))
                                   .setDepth(1))
                    .setArrayLayers(1)
                    .setPQueueFamilyIndices(indices.data())
                    .setQueueFamilyIndexCount(indices.size())
                    .setSharingMode(indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent)
                    .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
                    .setImageType(vk::ImageType::e2D)
                    .setMipLevels(1)
                    .setTiling(vk::ImageTiling::eOptimal)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setSamples(vk::SampleCountFlagBits::e1),
                "OffscreenRenderToImagesDepth")
            .setBaseFormat(this->findDepthFormat(device))
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e2D)
                             .setFormat(this->findDepthFormat(device))
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)))
            .setSamplerInfo(
                vk::SamplerCreateInfo()
                    .setAnisotropyEnable(true)
                    .setMaxAnisotropy(
                        star::StarTexture::SelectAnisotropyLevel(device.getPhysicalDevice().getProperties()))
                    .setMagFilter(star::StarTexture::SelectTextureFiltering(device.getPhysicalDevice().getProperties()))
                    .setMinFilter(star::StarTexture::SelectTextureFiltering(device.getPhysicalDevice().getProperties()))
                    .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                    .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                    .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                    .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                    .setUnnormalizedCoordinates(VK_FALSE)
                    .setCompareEnable(VK_FALSE)
                    .setCompareOp(vk::CompareOp::eAlways)
                    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                    .setMipLodBias(0.0f)
                    .setMinLod(0.0f)
                    .setMaxLod(0.0f));

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());

        auto oneTimeSetup = device.beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back()->getVulkanImage();
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
