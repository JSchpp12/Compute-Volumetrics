#include "OffscreenRenderer.hpp"

#include "Allocator.hpp"

OffscreenRenderer::OffscreenRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                     std::vector<std::shared_ptr<star::StarObject>> objects,
                                     std::vector<std::shared_ptr<star::Light>> lights,
                                     std::vector<star::Handle> &cameraInfos)
    : star::core::renderer::Renderer(context, numFramesInFlight, lights, cameraInfos, objects)
{
}

OffscreenRenderer::OffscreenRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                     std::vector<std::shared_ptr<star::StarObject>> objects,
                                     std::vector<std::shared_ptr<star::Light>> lights,
                                     std::shared_ptr<star::StarCamera> camera)
    : star::core::renderer::Renderer(context, numFramesInFlight, lights, camera, objects)
{
}

void OffscreenRenderer::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex, const uint64_t &frameIndex)
{
    // need to transition the image from general to color attachment
    // also get ownership back
    if (!this->isFirstPass)
    {
        std::array<const vk::ImageMemoryBarrier2, 2> backFromCompute{
            vk::ImageMemoryBarrier2()
                .setOldLayout(vk::ImageLayout::eGeneral)
                .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setSrcQueueFamilyIndex(*this->computeQueueFamilyIndex)
                .setDstQueueFamilyIndex(*this->graphicsQueueFamilyIndex)
                .setImage(this->renderToImages.at(frameInFlightIndex)->getVulkanImage())
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
                .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setBaseArrayLayer(0)
                                         .setLevelCount(1)
                                         .setLayerCount(1)),
            vk::ImageMemoryBarrier2()
                .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setSrcQueueFamilyIndex(*this->computeQueueFamilyIndex)
                .setDstQueueFamilyIndex(*this->graphicsQueueFamilyIndex)
                .setImage(this->renderToDepthImages.at(frameInFlightIndex)->getVulkanImage())
                .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
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

        commandBuffer.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(backFromCompute));
    }

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

    this->recordRenderingCalls(commandBuffer, frameInFlightIndex, frameIndex);

    commandBuffer.endRendering();

    // pass images back to compute queue family

    {
        std::array<const vk::ImageMemoryBarrier2, 2> toCompute{
            vk::ImageMemoryBarrier2()
                .setImage(this->renderToImages.at(frameInFlightIndex)->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setNewLayout(vk::ImageLayout::eGeneral)
                .setSrcQueueFamilyIndex(*this->graphicsQueueFamilyIndex)
                .setDstQueueFamilyIndex(*this->computeQueueFamilyIndex)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
                .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)),
            vk::ImageMemoryBarrier2()
                .setImage(this->renderToDepthImages.at(frameInFlightIndex)->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSrcQueueFamilyIndex(*this->graphicsQueueFamilyIndex)
                .setDstQueueFamilyIndex(*this->computeQueueFamilyIndex)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eLateFragmentTests |
                                 vk::PipelineStageFlagBits2::eEarlyFragmentTests)
                .setSrcAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setDstAccessMask(vk::AccessFlagBits2::eNone)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1))};

        const auto depInfo =
            vk::DependencyInfo().setPImageMemoryBarriers(&toCompute.front()).setImageMemoryBarrierCount(2);

        commandBuffer.pipelineBarrier2(depInfo);
    }

    if (this->isFirstPass)
    {
        this->firstFramePassCounter--;

        if (this->firstFramePassCounter == 0)
        {
            this->isFirstPass = false;
        }
    }
}

void OffscreenRenderer::initResources(star::core::device::DeviceContext &device, const int &numFramesInFlight,
                                      const vk::Extent2D &screenSize)
{
    {
        this->graphicsQueueFamilyIndex = std::make_unique<uint32_t>(
            device.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex());
        const uint32_t computeQueueIndex =
            device.getDevice().getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex();

        if (*this->graphicsQueueFamilyIndex != computeQueueIndex)
        {
            this->computeQueueFamilyIndex = std::make_unique<uint32_t>(uint32_t(computeQueueIndex));
        }
    }

    this->firstFramePassCounter = uint32_t(numFramesInFlight);

    star::core::renderer::Renderer::initResources(device, numFramesInFlight, screenSize);
}

std::vector<std::unique_ptr<star::StarTextures::Texture>> OffscreenRenderer::createRenderToImages(
    star::core::device::DeviceContext &device, const int &numFramesInFlight)
{
    std::vector<std::unique_ptr<star::StarTextures::Texture>> newRenderToImages =
        std::vector<std::unique_ptr<star::StarTextures::Texture>>();

    vk::Format colorFormat = getColorAttachmentFormat(device);

    auto builder =
        star::StarTextures::Texture::Builder(device.getDevice().getVulkanDevice(),
                                             device.getDevice().getAllocator().get())
            .setCreateInfo(star::Allocator::AllocationBuilder()
                               .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                               .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                               .build(),
                           vk::ImageCreateInfo()
                               .setExtent(vk::Extent3D()
                                              .setWidth(static_cast<int>(this->swapChainExtent->width))
                                              .setHeight(static_cast<int>(this->swapChainExtent->height))
                                              .setDepth(1))
                               .setSharingMode(vk::SharingMode::eExclusive)
                               .setArrayLayers(1)
                               .setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage)
                               .setImageType(vk::ImageType::e2D)
                               .setMipLevels(1)
                               .setTiling(vk::ImageTiling::eOptimal)
                               .setInitialLayout(vk::ImageLayout::eUndefined)
                               .setSamples(vk::SampleCountFlagBits::e1),
                           "OffscreenRenderToImages")
            .setBaseFormat(colorFormat)
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e2D)
                             .setFormat(colorFormat)
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)));

    // if (colorFormat != vk::Format::eR8G8B8A8Unorm){
    //     builder.addViewInfo(
    //         vk::ImageViewCreateInfo()
    //             .setViewType(vk::ImageViewType::e2D)
    //             .setFormat(vk::Format::eR8G8B8A8Unorm)
    //             .setSubresourceRange(vk::ImageSubresourceRange()
    //                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
    //                 .setBaseArrayLayer(0)
    //                 .setLayerCount(1)
    //                 .setBaseMipLevel(0)
    //                 .setLevelCount(1))
    //     );
    // }

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());

        auto oneTimeSetup = device.getDevice().beginSingleTimeCommands();

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

        oneTimeSetup->buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,             // which pipeline stages should
                                                               // occurr before barrier
            vk::PipelineStageFlagBits::eColorAttachmentOutput, // pipeline stage in
                                                               // which operations will
                                                               // wait on the barrier
            {}, {}, nullptr, barrier);

        device.getDevice().endSingleTimeCommands(std::move(oneTimeSetup));
    }

    return newRenderToImages;
}

std::vector<std::unique_ptr<star::StarTextures::Texture>> OffscreenRenderer::createRenderToDepthImages(
    star::core::device::DeviceContext &device, const int &numFramesInFlight)
{
    std::vector<std::unique_ptr<star::StarTextures::Texture>> newRenderToImages =
        std::vector<std::unique_ptr<star::StarTextures::Texture>>();

    const vk::Format depthFormat = this->getDepthAttachmentFormat(device);

    auto builder =
        star::StarTextures::Texture::Builder(device.getDevice().getVulkanDevice(),
                                             device.getDevice().getAllocator().get())
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
                    .setSharingMode(vk::SharingMode::eExclusive)
                    .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
                    .setImageType(vk::ImageType::e2D)
                    .setMipLevels(1)
                    .setTiling(vk::ImageTiling::eOptimal)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setSamples(vk::SampleCountFlagBits::e1),
                "OffscreenRenderToImagesDepth")
            .setBaseFormat(depthFormat)
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e2D)
                             .setFormat(depthFormat)
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)))
            .setSamplerInfo(vk::SamplerCreateInfo()
                                .setAnisotropyEnable(true)
                                .setMaxAnisotropy(star::StarTextures::Texture::SelectAnisotropyLevel(
                                    device.getDevice().getPhysicalDevice().getProperties()))
                                .setMagFilter(star::StarTextures::Texture::SelectTextureFiltering(
                                    device.getDevice().getPhysicalDevice().getProperties()))
                                .setMinFilter(star::StarTextures::Texture::SelectTextureFiltering(
                                    device.getDevice().getPhysicalDevice().getProperties()))
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

        auto oneTimeSetup = device.getDevice().beginSingleTimeCommands();

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

        oneTimeSetup->buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, // which pipeline stages should
                                                                                      // occurr before barrier
                                               vk::PipelineStageFlagBits::eEarlyFragmentTests, // pipeline stage in
                                                                                               // which operations will
                                                                                               // wait on the barrier
                                               {}, {}, nullptr, barrier);

        device.getDevice().endSingleTimeCommands(std::move(oneTimeSetup));
    }

    return newRenderToImages;
}

std::vector<std::shared_ptr<star::StarBuffers::Buffer>> OffscreenRenderer::createDepthBufferContainers(
    star::core::device::DeviceContext &device)
{
    return std::vector<std::shared_ptr<star::StarBuffers::Buffer>>();
}

star::core::device::manager::ManagerCommandBuffer::Request OffscreenRenderer::getCommandBufferRequest()
{
    return star::core::device::manager::ManagerCommandBuffer::Request{
        .recordBufferCallback =
            std::bind(&OffscreenRenderer::recordCommandBuffer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        .order = star::Command_Buffer_Order::before_render_pass,
        .orderIndex = star::Command_Buffer_Order_Index::first,
        .waitStage = vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .willBeSubmittedEachFrame = true,
        .recordOnce = false};
}

vk::RenderingAttachmentInfo OffscreenRenderer::prepareDynamicRenderingInfoDepthAttachment(const int &frameInFlightIndex)
{
    vk::RenderingAttachmentInfoKHR depthAttachmentInfo{};
    depthAttachmentInfo.imageView = this->renderToDepthImages[frameInFlightIndex]->getImageView();
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachmentInfo.clearValue = vk::ClearValue{vk::ClearDepthStencilValue{1.0f}};

    return depthAttachmentInfo;
}

vk::Format OffscreenRenderer::getColorAttachmentFormat(star::core::device::DeviceContext &device) const
{
    vk::Format selectedFormat = vk::Format();
    if (!device.getDevice().findSupportedFormat(
            {vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Unorm}, vk::ImageTiling::eOptimal,
            {vk::FormatFeatureFlagBits::eColorAttachment | vk::FormatFeatureFlagBits::eStorageImage}, selectedFormat))
    {
        throw std::runtime_error("Failed to find supported color attachment format");
    }

    return selectedFormat;
}

vk::Format OffscreenRenderer::getDepthAttachmentFormat(star::core::device::DeviceContext &device) const
{
    vk::Format selectedFormat = vk::Format();
    if (!device.getDevice().findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            {vk::FormatFeatureFlagBits::eDepthStencilAttachment | vk::FormatFeatureFlagBits::eSampledImage},
            selectedFormat))
    {
        throw std::runtime_error("Failed to find supported depth format");
    }

    return selectedFormat;
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
