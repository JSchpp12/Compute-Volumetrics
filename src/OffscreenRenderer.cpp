#include "OffscreenRenderer.hpp"

#include "Allocator.hpp"

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

static std::vector<star::Handle> CreateSemaphores(star::common::EventBus &evtBus,
                                                  const star::common::FrameTracker &ft) noexcept
{
    const size_t num = static_cast<size_t>(ft.getSetup().getNumFramesInFlight());

    auto handles = std::vector<star::Handle>(num);
    for (size_t i{0}; i < handles.size(); i++)
    {
        void *r = nullptr;
        evtBus.emit(star::core::device::system::event::ManagerRequest(
            star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                star::core::device::manager::GetSemaphoreEventTypeName),
            star::core::device::manager::SemaphoreRequest{true}, handles[i], &r));

        if (r == nullptr)
        {
            STAR_THROW("Unable to create new semaphore");
        }
    }

    return handles;
}

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

    commandBuffer.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(prepImages));

    vk::Viewport viewport = this->prepareRenderingViewport(m_renderingContext.targetResolution);
    commandBuffer.setViewport(0, viewport);

    this->recordPreRenderPassCommands(commandBuffer, frameTracker);

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

    this->recordRenderingCalls(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(),
                               frameTracker.getCurrent().getGlobalFrameCounter());

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

void OffscreenRenderer::waitForSemaphore(const star::common::FrameTracker &ft) const
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
        {
            STAR_THROW("Failed to wait for timeline semaphores");
        }
    }
}

void OffscreenRenderer::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                            const star::common::FrameTracker &ft, const uint64_t &frameIndex)
{
    waitForSemaphore(ft);
    this->star::core::renderer::DefaultRenderer::recordCommandBuffer(commandBuffer, ft, frameIndex);
}

vk::RenderingAttachmentInfo OffscreenRenderer::prepareDynamicRenderingInfoColorAttachment(
    const star::common::FrameTracker &frameTracker)
{
    const auto tmp = this->DefaultRenderer::prepareDynamicRenderingInfoColorAttachment(frameTracker);

    const size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    return vk::RenderingAttachmentInfo()
        .setImageView(m_renderingContext.recordDependentImage.get(m_renderToImages[index])->getImageView())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearValue().setColor({1.0f, 1.0f, 1.0f, 1.0f}));
}

std::vector<star::StarTextures::Texture> OffscreenRenderer::createRenderToImages(
    star::core::device::DeviceContext &device, const uint8_t &numFramesInFlight)
{
    auto newRenderToImages = std::vector<star::StarTextures::Texture>();

    vk::Format colorFormat = getColorAttachmentFormat(device);

    int width, height;
    {
        const auto &resolution = device.getEngineResolution();
        star::common::casts::SafeCast<vk::DeviceSize, int>(resolution.width, width);
        star::common::casts::SafeCast<vk::DeviceSize, int>(resolution.height, height);
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
        star::common::casts::SafeCast<vk::DeviceSize, int>(resolution.width, width);
        star::common::casts::SafeCast<vk::DeviceSize, int>(resolution.height, height);
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
        device.getEventBus(), device.getGraphicsManagers().queueManager, star::Queue_Type::Tcompute);
    assert(queue != nullptr);

    for (int i = 0; i < numFramesInFlight; i++)
    {
        newRenderToImages.emplace_back(builder.build());
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
        .recordOnce = false,
        .overrideBufferSubmissionCallback = std::bind(
            &OffscreenRenderer::submitBuffer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
            std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7)};
}

void OffscreenRenderer::prepRender(star::common::IDeviceContext &c)
{
    auto &context = static_cast<star::core::device::DeviceContext &>(c);

    m_cmdBus = &context.getCmdBus();
    m_device = context.getDevice().getVulkanDevice();

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

    m_timelineSemaphores = CreateSemaphores(context.getEventBus(), context.getFrameTracker());
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

static void GetNeighborSemaphoresFromLastPass(const star::core::CommandBus &cmdBus, const star::Handle &registration,
                                              std::vector<vk::Semaphore> &semaphores,
                                              std::vector<uint64_t> &previousSignalValues) noexcept
{
    auto cmd = star::command_order::GetPassInfo{registration};
    cmdBus.submit(cmd);

    for (const auto &edge : *cmd.getReply().get().edges)
    {
        if (edge.producer == registration)
        {
            auto nCmd = star::command_order::GetPassInfo{registration};
            cmdBus.submit(nCmd);

            semaphores.emplace_back(nCmd.getReply().get().signaledSemaphore);
            previousSignalValues.emplace_back(nCmd.getReply().get().currentSignalValue);
        }
    }
}

vk::Semaphore OffscreenRenderer::submitBuffer(star::StarCommandBuffer &buffer,
                                              const star::common::FrameTracker &frameTracker,
                                              std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                              std::vector<vk::Semaphore> dataSemaphores,
                                              std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                              std::vector<std::optional<uint64_t>> previousSignaledValues,
                                              star::StarQueue &queue)
{
    const size_t ii = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
    assert(m_cmdBus != nullptr);

    std::vector<vk::Semaphore> nSemaphore;
    std::vector<uint64_t> nSemaphoreValues;
    vk::Semaphore mySemaphore{VK_NULL_HANDLE};
    uint64_t mySemaphoreSignalValue{0};
    {
        auto cmd = star::command_order::GetPassInfo{m_commandBuffer};
        m_cmdBus->submit(cmd);
        mySemaphore = cmd.getReply().get().signaledSemaphore;
        mySemaphoreSignalValue = cmd.getReply().get().toSignalValue;

        for (const auto &edge : *cmd.getReply().get().edges)
        {
            if (edge.producer == m_commandBuffer)
            {
                auto nCmd = star::command_order::GetPassInfo{edge.consumer};
                m_cmdBus->submit(nCmd);

                nSemaphore.emplace_back(nCmd.getReply().get().signaledSemaphore);
                nSemaphoreValues.emplace_back(nCmd.getReply().get().currentSignalValue);
            }
        }
    }

    const auto cbInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(
        buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()));

    std::vector<vk::SemaphoreSubmitInfo> waitInfo;
    for (size_t i{0}; i < nSemaphore.size(); i++)
    {
        waitInfo.push_back(vk::SemaphoreSubmitInfo()
                               .setSemaphore(nSemaphore[i])
                               .setValue(nSemaphoreValues[i])
                               .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
    }

    assert(dataSemaphores.size() == dataWaitPoints.size());
    for (size_t i{0}; i < dataWaitPoints.size(); i++)
    {
        waitInfo.emplace_back(vk::SemaphoreSubmitInfo()
                                  .setSemaphore(dataSemaphores[i])
                                  .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
    }

    vk::Semaphore binarySemaphore{buffer.getCompleteSemaphores()[ii]};
    const vk::SemaphoreSubmitInfo signalInfo[1]{vk::SemaphoreSubmitInfo()
                                                    .setSemaphore(mySemaphore)
                                                    .setValue(mySemaphoreSignalValue)
                                                    .setStageMask(vk::PipelineStageFlagBits2::eAllCommands)};

    const auto submitInfo =
        vk::SubmitInfo2().setWaitSemaphoreInfos(waitInfo).setCommandBufferInfos(cbInfo).setSignalSemaphoreInfos(
            signalInfo);

    queue.getVulkanQueue().submit2(submitInfo);

    return vk::Semaphore();
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
