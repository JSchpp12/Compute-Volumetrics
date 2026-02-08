#include "OffscreenRenderer.hpp"

#include "Allocator.hpp"

#include <starlight/core/helper/command_buffer/CommandBufferHelpers.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>
#include <starlight/command/command_order/DeclarePass.hpp>

OffscreenRenderer::OffscreenRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                     std::vector<std::shared_ptr<star::StarObject>> objects,
                                     std::shared_ptr<std::vector<star::Light>> lights,
                                     std::shared_ptr<star::StarCamera> camera)
    : star::core::renderer::DefaultRenderer(context, numFramesInFlight, std::move(lights), camera, objects)
{
}

void OffscreenRenderer::recordCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                                       const uint64_t &frameIndex)
{
    size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
    star::StarTextures::Texture *colorTex = m_renderingContext.recordDependentImage.get(m_renderToImages[index]);
    star::StarTextures::Texture *depthTex = m_renderingContext.recordDependentImage.get(m_renderToDepthImages[index]);

    // need to transition the image from general to color attachment
    // also get ownership back
    if (!this->isFirstPass)
    {
        std::array<const vk::ImageMemoryBarrier2, 2> backFromCompute{
            vk::ImageMemoryBarrier2()
                .setOldLayout(vk::ImageLayout::eGeneral)
                .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex)
                .setDstQueueFamilyIndex(this->graphicsQueueFamilyIndex)
                .setImage(colorTex->getVulkanImage())
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
                .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex)
                .setDstQueueFamilyIndex(this->graphicsQueueFamilyIndex)
                .setImage(depthTex->getVulkanImage())
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

    vk::Viewport viewport = this->prepareRenderingViewport(m_renderingContext.targetResolution);
    commandBuffer.setViewport(0, viewport);

    this->recordPreRenderPassCommands(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(), frameIndex);

    {
        // dynamic rendering used...so dont need all that extra stuff
        vk::RenderingAttachmentInfo colorAttachmentInfo = prepareDynamicRenderingInfoColorAttachment(frameTracker);
        vk::RenderingAttachmentInfo depthAttachmentInfo = prepareDynamicRenderingInfoDepthAttachment(frameTracker);

        auto renderArea = vk::Rect2D{vk::Offset2D{}, m_renderingContext.targetResolution};
        vk::RenderingInfoKHR renderInfo{};
        renderInfo.renderArea = renderArea;
        renderInfo.layerCount = 1;
        renderInfo.pDepthAttachment = &depthAttachmentInfo;
        renderInfo.pColorAttachments = &colorAttachmentInfo;
        renderInfo.colorAttachmentCount = 1;
        commandBuffer.beginRendering(renderInfo);
    }

    this->recordRenderingCalls(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(), frameIndex);

    commandBuffer.endRendering();

    // pass images back to compute queue family

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
                .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
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

std::vector<star::StarTextures::Texture> OffscreenRenderer::createRenderToImages(
    star::core::device::DeviceContext &device, const uint8_t &numFramesInFlight)
{
    auto newRenderToImages = std::vector<star::StarTextures::Texture>();

    vk::Format colorFormat = getColorAttachmentFormat(device);

    int width, height;
    {
        const auto &resolution = device.getEngineResolution();
        star::common::helper::SafeCast<vk::DeviceSize, int>(resolution.width, width);
        star::common::helper::SafeCast<vk::DeviceSize, int>(resolution.height, height);
    }

    auto builder =
        star::StarTextures::Texture::Builder(device.getDevice().getVulkanDevice(),
                                             device.getDevice().getAllocator().get())
            .setCreateInfo(star::Allocator::AllocationBuilder()
                               .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                               .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                               .build(),
                           vk::ImageCreateInfo()
                               .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
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

    auto *queue = star::core::helper::GetEngineDefaultQueue(
        device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tgraphics);
    assert(queue != nullptr);

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());

        auto oneTimeSetup = star::core::helper::BeginSingleTimeCommands(device.getDevice(), device.getEventBus(),
                                                                        device.getManagerCommandBuffer().m_manager,
                                                                        star::Queue_Type::Tgraphics);

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back().getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
        barrier.subresourceRange.levelCount = 1;   // image is not an array
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        oneTimeSetup.buffer().pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,             // which pipeline stages should
                                                               // occurr before barrier
            vk::PipelineStageFlagBits::eColorAttachmentOutput, // pipeline stage in
                                                               // which operations will
                                                               // wait on the barrier
            {}, {}, nullptr, barrier);

        star::core::helper::EndSingleTimeCommands(*queue, std::move(oneTimeSetup));
    }

    return newRenderToImages;
}

std::vector<star::StarTextures::Texture> OffscreenRenderer::createRenderToDepthImages(
    star::core::device::DeviceContext &device, const uint8_t &numFramesInFlight)
{
    std::vector<star::StarTextures::Texture> newRenderToImages;

    const vk::Format depthFormat = this->getDepthAttachmentFormat(device);

    int width, height;
    {
        const auto &resolution = device.getEngineResolution();
        star::common::helper::SafeCast<vk::DeviceSize, int>(resolution.width, width);
        star::common::helper::SafeCast<vk::DeviceSize, int>(resolution.height, height);
    }

    auto builder =
        star::StarTextures::Texture::Builder(device.getDevice().getVulkanDevice(),
                                             device.getDevice().getAllocator().get())
            .setCreateInfo(
                star::Allocator::AllocationBuilder()
                    .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                    .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .build(),
                vk::ImageCreateInfo()
                    .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
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

    auto *queue = star::core::helper::GetEngineDefaultQueue(
        device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tgraphics);
    assert(queue != nullptr);

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());

        auto oneTimeSetup = star::core::helper::BeginSingleTimeCommands(device.getDevice(), device.getEventBus(),
                                                                        device.getManagerCommandBuffer().m_manager,
                                                                        star::Queue_Type::Tgraphics);

        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = newRenderToImages.back().getVulkanImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
        barrier.subresourceRange.levelCount = 1;   // image is not an array
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        oneTimeSetup.buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, // which pipeline stages should
                                                                                     // occurr before barrier
                                              vk::PipelineStageFlagBits::eEarlyFragmentTests, // pipeline stage in
                                                                                              // which operations will
                                                                                              // wait on the barrier
                                              {}, {}, nullptr, barrier);

        star::core::helper::EndSingleTimeCommands(*queue, std::move(oneTimeSetup));
    }

    return newRenderToImages;
}

star::core::device::manager::ManagerCommandBuffer::Request OffscreenRenderer::getCommandBufferRequest()
{
    return star::core::device::manager::ManagerCommandBuffer::Request{
        .recordBufferCallback = std::bind(&OffscreenRenderer::recordCommandBuffer, this, std::placeholders::_1,
                                          std::placeholders::_2, std::placeholders::_3),
        .order = star::Command_Buffer_Order::before_render_pass,
        .orderIndex = star::Command_Buffer_Order_Index::first,
        .waitStage = vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .willBeSubmittedEachFrame = true,
        .recordOnce = false};
}

void OffscreenRenderer::prepRender(star::common::IDeviceContext &c)
{
    auto &context = static_cast<star::core::device::DeviceContext &>(c);
    {
        this->graphicsQueueFamilyIndex =
            star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                      star::Queue_Type::Tgraphics)
                ->getParentQueueFamilyIndex();

        this->computeQueueFamilyIndex =
            star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                      star::Queue_Type::Tcompute)
                ->getParentQueueFamilyIndex();
    }

    this->firstFramePassCounter = uint32_t(context.getFrameTracker().getSetup().getNumFramesInFlight());

    star::core::renderer::DefaultRenderer::prepRender(c);

    auto cmd = star::command_order::DeclarePass(this->m_commandBuffer, this->graphicsQueueFamilyIndex);
    context.begin().set(cmd).submit(); 
}

vk::RenderingAttachmentInfo OffscreenRenderer::prepareDynamicRenderingInfoDepthAttachment(
    const star::common::FrameTracker &frameTracker)
{
    size_t i = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
    return vk::RenderingAttachmentInfoKHR()
        .setImageView(m_renderingContext.recordDependentImage.get(m_renderToDepthImages[i])->getImageView())
        .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue{1.0f}));
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
