#include "VolumeRenderer.hpp"

#include "AABBTransfer.hpp"
#include "CameraInfo.hpp"
#include "ConfigFile.hpp"
#include "FogControlInfo.hpp"
#include "FogData.hpp"
#include "LevelSetData.hpp"
#include "ManagerRenderResource.hpp"
#include "RandomValueTexture.hpp"
#include "VDBTransfer.hpp"
#include "VolumeDirectoryProcessor.hpp"
#include "VolumeRendererCreateDescriptorsPolicy.hpp"
#include "core/device/managers/DescriptorPool.hpp"
#include "event/EnginePhaseComplete.hpp"
#include "wrappers/graphics/policies/CreateDescriptorsOnEventPolicy.hpp"
#include "wrappers/graphics/policies/SubmitDescriptorRequestsPolicy.hpp"

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

#include <star_common/HandleTypeRegistry.hpp>

VolumeRenderer::VolumeRenderer(std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceManagerInfo,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceNormalInfo,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalInfoBuffers,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneLightInfoBuffers,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneLightList,
                               OffscreenRenderer *offscreenRenderer, std::string vdbFilePath,
                               std::shared_ptr<FogInfo> fogControlInfo, const std::shared_ptr<star::StarCamera> camera,
                               const std::array<glm::vec4, 2> &aabbBounds)
    : m_infoManagerInstanceModel(instanceManagerInfo), m_infoManagerInstanceNormal(instanceNormalInfo),
      m_infoManagerGlobalCamera(globalInfoBuffers), m_infoManagerSceneLightInfo(sceneLightInfoBuffers),
      m_infoManagerSceneLightList(sceneLightList), m_offscreenRenderer(offscreenRenderer),
      m_vdbFilePath(std::move(vdbFilePath)), m_fogController(fogControlInfo), aabbBounds(aabbBounds), camera(camera),
      volumeTexture(volumeTexture)
{
}

void VolumeRenderer::init(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    using registry = star::common::HandleTypeRegistry;

    auto submitter = std::make_shared<star::wrappers::graphics::policies::SubmitDescriptorRequestsPolicy>(
        getDescriptorRequests(numFramesInFlight));

    submitter->init(context.getEventBus());

    if (!registry::instance().contains(star::event::GetEnginePhaseCompleteLoadTypeName()))
    {
        registry::instance().registerType(star::event::GetEnginePhaseCompleteLoadTypeName());
    }
    auto trigger = star::wrappers::graphics::policies::CreateDescriptorsOnEventPolicy<
                       VolumeRendererCreateDescriptorsPolicy>::Builder(context.getEventBus())
                       .setEventType(registry::instance().getTypeGuaranteedExist(
                           star::event::GetEnginePhaseCompleteLoadTypeName()))
                       .setPolicy(VolumeRendererCreateDescriptorsPolicy{&context.getDeviceID(),
                                                                        &m_fogController,
                                                                        &aabbInfoBuffers,
                                                                        &m_offscreenRenderer->getRenderToColorImages(),
                                                                        &m_offscreenRenderer->getRenderToDepthImages(),
                                                                        &computeWriteToImages,
                                                                        &computeRayDistanceBuffers,
                                                                        &computeRayAtCutoffDistanceBuffers,
                                                                        &nanoVDBPipeline_hitBoundingBox,
                                                                        &nanoVDBPipeline_surface,
                                                                        &marchedPipeline,
                                                                        &linearPipeline,
                                                                        &expPipeline,
                                                                        &cameraShaderInfo,
                                                                        &vdbInfoSDF,
                                                                        &vdbInfoFog,
                                                                        &randomValueTexture,
                                                                        &SDFShaderInfo,
                                                                        &VolumeShaderInfo,
                                                                        &computePipelineLayout,
                                                                        m_infoManagerInstanceModel,
                                                                        m_infoManagerInstanceNormal,
                                                                        m_infoManagerGlobalCamera,
                                                                        m_infoManagerSceneLightInfo,
                                                                        m_infoManagerSceneLightList,
                                                                        &context.getDevice(),
                                                                        &context.getGraphicsManagers(),
                                                                        &context.getManagerRenderResource(),
                                                                        numFramesInFlight})
                       .buildShared();
}

bool VolumeRenderer::isRenderReady(star::core::device::DeviceContext &context)
{
    if (isReady)
    {
        return true;
    }

    if (context.getPipelineManager().get(marchedPipeline)->isReady() &&
        context.getPipelineManager().get(linearPipeline)->isReady() &&
        context.getPipelineManager().get(expPipeline)->isReady() &&
        context.getPipelineManager().get(nanoVDBPipeline_hitBoundingBox)->isReady() &&
        context.getPipelineManager().get(nanoVDBPipeline_surface)->isReady())
    {
        isReady = true;
    }

    return isReady;
}

void VolumeRenderer::frameUpdate(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    updateRenderingContext(context, frameInFlightIndex);

    if (isRenderReady(context))
    {
        updateDependentData(context, frameInFlightIndex);
        gatherDependentExternalDataOrderingInfo(context, frameInFlightIndex);
    }
}

void VolumeRenderer::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                         const star::common::FrameTracker &frameTracker, const uint64_t &frameIndex)
{
    commandBuffer.begin(frameTracker.getCurrent().getFrameInFlightIndex());

    recordCommands(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()), frameTracker, frameIndex);

    commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()).end();
}

std::unique_ptr<star::command_order::get_pass_info::GatheredPassInfo> VolumeRenderer::getTransferNeighborInfo()
{
    std::unique_ptr<star::command_order::get_pass_info::GatheredPassInfo> transferNeighborInfo = nullptr;

    auto getCmd = star::command_order::GetPassInfo(m_commandBuffer);
    m_checkForDepsSubmitter.update(getCmd).submit();

    // command deps
    const star::command_order::get_pass_info::GatheredPassInfo &ele = getCmd.getReply().get();

    if (ele.edges != nullptr)
    {
        const auto &edge = ele.edges->front();
        const star::Handle f = edge.producer == m_commandBuffer ? edge.consumer : edge.producer;

        auto nGetCmd = star::command_order::GetPassInfo(f);
        m_checkForDepsSubmitter.update(nGetCmd).submit();

        transferNeighborInfo =
            std::make_unique<star::command_order::get_pass_info::GatheredPassInfo>(nGetCmd.getReply().get());
    }

    return transferNeighborInfo;
}

void VolumeRenderer::recordCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                                    const uint64_t &frameIndex)
{
    auto tNeighbor = getTransferNeighborInfo();

    // check if other dep was run this frame
    {
        const bool getFromTransfer = tNeighbor != nullptr && tNeighbor->wasProcessedOnLastFrame->at(
                                                                 frameTracker.getCurrent().getFrameInFlightIndex());
        addPreComputeMemoryBarriers(commandBuffer, frameTracker, getFromTransfer);
    }

    if (isReady)
    {
        m_renderingContext.pipeline->bind(commandBuffer);

        std::vector<vk::DescriptorSet> sets;
        if (this->currentFogType == FogType::marched)
        {
            sets = this->VolumeShaderInfo->getDescriptors(frameTracker.getCurrent().getFrameInFlightIndex());
        }
        else
        {
            sets = this->SDFShaderInfo->getDescriptors(frameTracker.getCurrent().getFrameInFlightIndex());
        }
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *this->computePipelineLayout, 0,
                                         static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

        commandBuffer.dispatch(this->workgroupSize.x, this->workgroupSize.y, 1);
    }

    {
        const bool giveToTransfer = tNeighbor != nullptr && tNeighbor->isTriggeredThisFrame;
        addPostComputeMemoryBarriers(commandBuffer, frameTracker, giveToTransfer);
    }
}

void VolumeRenderer::recordQueueFamilyInfo(star::core::device::DeviceContext &context)
{
    this->computeQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tcompute)
            ->getParentQueueFamilyIndex();

    this->graphicsQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();
    this->transferQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Ttransfer)
            ->getParentQueueFamilyIndex();
}

void VolumeRenderer::prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &screensize,
                                const uint8_t &numFramesInFlight)
{
    m_checkForDepsSubmitter = context.begin();
    m_checkForDepsSubmitter.setType(star::command_order::get_pass_info::GetPassInfoTypeName());

    recordQueueFamilyInfo(context);

    const auto camSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    this->cameraShaderInfo = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(camSemaphore)->semaphore,
        std::make_unique<CameraInfo>(
            this->camera, computeQueueFamilyIndex,
            context.getDevice().getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment),
        nullptr, true);

    const auto vdbSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    // need to find a way to tell what type the volume is...
    // dragon is level set
    const auto tmpDir = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::tmp_directory));
    VolumeDirectoryProcessor processor(m_vdbFilePath, std::move(tmpDir));
    processor.init();

    const auto &frontPath = processor.getProcessedFiles().front().getDataFilePath();
    // fog sim is fog
    this->vdbInfoSDF = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(vdbSemaphore)->semaphore,
        std::make_unique<VDBTransfer>(
            computeQueueFamilyIndex, std::make_unique<FogData>(frontPath.string(), openvdb::GridClass::GRID_LEVEL_SET)),
        nullptr, true);
    const auto vdbFogSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    this->vdbInfoFog = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(vdbFogSemaphore)->semaphore,
        std::make_unique<VDBTransfer>(
            computeQueueFamilyIndex,
            std::make_unique<FogData>(frontPath.string(), openvdb::GridClass::GRID_FOG_VOLUME)),
        nullptr, true);

    const auto randomSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    this->randomValueTexture = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(randomSemaphore)->semaphore,
        std::make_unique<RandomValueTexture>(screensize.width, screensize.height, this->computeQueueFamilyIndex,
                                             context.getDevice().getPhysicalDevice().getProperties()));

    this->workgroupSize = CalculateWorkGroupSize(screensize);

    {
        const size_t n = static_cast<size_t>(numFramesInFlight);
        this->computeWriteToImages = createComputeWriteToImages(context, screensize, n);
        this->computeRayDistanceBuffers =
            createComputeWriteToBuffers(context, screensize, sizeof(float), "RayDistanceBuffer", n);
        computeRayAtCutoffDistanceBuffers =
            createComputeWriteToBuffers(context, screensize, sizeof(uint32_t), "RayScissorBuffer", n);
    }

    m_fogController.prepRender(context, numFramesInFlight);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        const auto aabbSemaphore =
            context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

        this->aabbInfoBuffers.emplace_back(star::ManagerRenderResource::addRequest(
            context.getDeviceID(), context.getSemaphoreManager().get(aabbSemaphore)->semaphore,
            std::make_unique<AABBTransfer>(this->graphicsQueueFamilyIndex, aabbBounds)));

        const auto fogShaderInfoSemaphore =
            context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));
    }

    m_commandBuffer = context.getManagerCommandBuffer().submit(
        star::core::device::manager::ManagerCommandBuffer::Request{
            .recordBufferCallback = std::bind(&VolumeRenderer::recordCommandBuffer, this, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3),
            .order = star::Command_Buffer_Order::before_render_pass,
            .orderIndex = star::Command_Buffer_Order_Index::second,
            .type = star::Queue_Type::Tcompute,
            .waitStage = vk::PipelineStageFlagBits::eComputeShader,
            .willBeSubmittedEachFrame = true,
            .recordOnce = false},
        context.getFrameTracker().getSetup().getNumFramesInFlight());

    auto cmd = star::command_order::DeclarePass(m_commandBuffer, this->computeQueueFamilyIndex);
    context.begin().set(cmd).submit();

    for (size_t i = 0; i < static_cast<size_t>(numFramesInFlight); i++)
    {
        auto &ch = m_offscreenRenderer->getRenderToColorImages()[i];
        m_renderingContext.recordDependentImage.manualInsert(ch, &context.getImageManager().get(ch)->texture);
        auto &dh = m_offscreenRenderer->getRenderToDepthImages()[i];
        m_renderingContext.recordDependentImage.manualInsert(dh, &context.getImageManager().get(dh)->texture);
    }
}

void VolumeRenderer::cleanupRender(star::core::device::DeviceContext &context)
{
    this->SDFShaderInfo->cleanupRender(context.getDevice());
    this->SDFShaderInfo.reset();

    this->VolumeShaderInfo->cleanupRender(context.getDevice());
    this->VolumeShaderInfo.reset();

    for (auto &image : computeWriteToImages)
    {
        image->cleanupRender(context.getDevice().getVulkanDevice());
    }
    for (auto &buffer : computeRayDistanceBuffers)
    {
        buffer.cleanupRender(context.getDevice().getVulkanDevice());
    }
    for (auto &buffer : computeRayAtCutoffDistanceBuffers)
    {
        buffer.cleanupRender(context.getDevice().getVulkanDevice());
    }

    context.getDevice().getVulkanDevice().destroyPipelineLayout(*this->computePipelineLayout);
}

void VolumeRenderer::registerListenForEngineInitDone(star::common::EventBus &eventBus)
{
}

std::vector<std::pair<vk::DescriptorType, const uint32_t>> VolumeRenderer::getDescriptorRequests(
    const int &numFramesInFlight)
{
    return std::vector<std::pair<vk::DescriptorType, const uint32_t>>{
        std::make_pair(vk::DescriptorType::eStorageImage, 1 + (3 * numFramesInFlight * 50)),
        std::make_pair(vk::DescriptorType::eUniformBuffer, 1 + (4 * numFramesInFlight * 50)),
        std::make_pair(vk::DescriptorType::eStorageBuffer, 6 * numFramesInFlight),
        std::make_pair(vk::DescriptorType::eCombinedImageSampler, 800 * numFramesInFlight)};
}

void VolumeRenderer::recordDependentDataPipelineBarriers(vk::CommandBuffer &commandBuffer,
                                                         const uint8_t &frameInFlightIndex, const uint64_t &frameIndex)
{
}

void VolumeRenderer::gatherDependentExternalDataOrderingInfo(star::core::device::DeviceContext &context,
                                                             const uint8_t &frameInFlightIndex)
{
}

void VolumeRenderer::updateDependentData(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    vk::Semaphore dataSemaphore = VK_NULL_HANDLE;

    if (m_fogController.submitUpdateIfNeeded(context, frameInFlightIndex, dataSemaphore))
    {
        context.getManagerCommandBuffer()
            .m_manager.get(m_commandBuffer)
            .oneTimeWaitSemaphoreInfo.insert(m_fogController.getHandle(frameInFlightIndex), std::move(dataSemaphore),
                                             vk::PipelineStageFlagBits::eComputeShader);

        m_renderingContext.addBufferToRenderingContext(context, m_fogController.getHandle(frameInFlightIndex));
    }
}

void VolumeRenderer::updateRenderingContext(star::core::device::DeviceContext &context,
                                            const uint8_t &frameInFlightIndex)
{
    switch (this->currentFogType)
    {
    case (FogType::marched):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->marchedPipeline)->request.pipeline;
        break;
    case (FogType::linear):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->linearPipeline)->request.pipeline;
        break;
    case (FogType::exp):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->expPipeline)->request.pipeline;
        break;
    case (FogType::nano_boundingBox):
        m_renderingContext.pipeline =
            &context.getPipelineManager().get(this->nanoVDBPipeline_hitBoundingBox)->request.pipeline;
        break;
    case (FogType::nano_surface):
        m_renderingContext.pipeline =
            &context.getPipelineManager().get(this->nanoVDBPipeline_surface)->request.pipeline;
        break;
    default:
        throw std::runtime_error("Unsupported type");
    }

    // size_t index = static_cast<size_t>(frameInFlightIndex);
    // auto *colorImage = &context.getImageManager().get(m_offscreenRenderer->getRenderToColorImages()[index])->texture;
    // auto *depthImage = &context.getImageManager().get(m_offscreenRenderer->getRenderToDepthImages()[index])->texture;
    // rContext.recordDependentImage.manualInsert(m_offscreenRenderer->getRenderToColorImages()[index], colorImage);
    // rContext.recordDependentImage.manualInsert(m_offscreenRenderer->getRenderToDepthImages()[index], depthImage);

    // return rContext;
}

glm::uvec2 VolumeRenderer::CalculateWorkGroupSize(const vk::Extent2D &screenSize)
{
    const uint32_t width = static_cast<uint32_t>(std::ceil(static_cast<float>(screenSize.width) / 8.0f));
    const uint32_t height = static_cast<uint32_t>(std::ceil(static_cast<float>(screenSize.height) / 8.0f));

    return glm::uvec2{width, height};
}

std::vector<std::shared_ptr<star::StarTextures::Texture>> VolumeRenderer::createComputeWriteToImages(
    star::core::device::DeviceContext &context, const vk::Extent2D &screenSize, const size_t &numToCreate) const
{
    auto textures = std::vector<std::shared_ptr<star::StarTextures::Texture>>(numToCreate);

    std::vector<uint32_t> indices = {this->graphicsQueueFamilyIndex};
    if (this->computeQueueFamilyIndex != this->graphicsQueueFamilyIndex)
    {
        indices.push_back(this->computeQueueFamilyIndex);
    }

    auto builder =
        star::StarTextures::Texture::Builder(context.getDevice().getVulkanDevice(),
                                             context.getDevice().getAllocator().get())
            .setCreateInfo(star::Allocator::AllocationBuilder()
                               .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                               .setUsage(VMA_MEMORY_USAGE_AUTO)
                               .build(),
                           vk::ImageCreateInfo()
                               .setExtent(vk::Extent3D()
                                              .setWidth(static_cast<uint32_t>(screenSize.width))
                                              .setHeight(static_cast<uint32_t>(screenSize.height))
                                              .setDepth(1))
                               .setPQueueFamilyIndices(&indices[0])
                               .setQueueFamilyIndexCount(2)
                               .setArrayLayers(1)
                               .setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled)
                               .setImageType(vk::ImageType::e2D)
                               .setMipLevels(1)
                               .setTiling(vk::ImageTiling::eOptimal)
                               .setInitialLayout(vk::ImageLayout::eUndefined)
                               .setSamples(vk::SampleCountFlagBits::e1)
                               .setSharingMode(vk::SharingMode::eExclusive),
                           "VolumeRendererImages")
            .setBaseFormat(vk::Format::eR8G8B8A8Unorm)
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e2D)
                             .setFormat(vk::Format::eR8G8B8A8Unorm)
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(1)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)))
            .setSamplerInfo(vk::SamplerCreateInfo()
                                .setAnisotropyEnable(true)
                                .setMaxAnisotropy(star::StarTextures::Texture::SelectAnisotropyLevel(
                                    context.getDevice().getPhysicalDevice().getProperties()))
                                .setMagFilter(star::StarTextures::Texture::SelectTextureFiltering(
                                    context.getDevice().getPhysicalDevice().getProperties()))
                                .setMinFilter(star::StarTextures::Texture::SelectTextureFiltering(
                                    context.getDevice().getPhysicalDevice().getProperties()))
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

    for (size_t i{0}; i < numToCreate; i++)
    {
        textures[i] = builder.buildShared();
    }

    return textures;
}

std::vector<star::StarBuffers::Buffer> VolumeRenderer::createComputeWriteToBuffers(
    star::core::device::DeviceContext &context, const vk::Extent2D &screenSize, const size_t &dataTypeSize,
    const std::string &debugName, const size_t &numToCreate) const
{
    auto buffers = std::vector<star::StarBuffers::Buffer>(numToCreate);

    const vk::DeviceSize bufferSize = screenSize.width * screenSize.height * dataTypeSize;
    auto builder =
        star::StarBuffers::Buffer::Builder(context.getDevice().getAllocator().get())
            .setAllocationCreateInfo(
                star::Allocator::AllocationBuilder()
                    .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                    .setUsage(VMA_MEMORY_USAGE_AUTO)
                    .build(),
                vk::BufferCreateInfo()
                    .setSharingMode(vk::SharingMode::eExclusive)
                    .setSize(bufferSize)
                    .setUsage(vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer),
                debugName)
            .setInstanceCount(1)
            .setInstanceSize(bufferSize);

    for (size_t i{0}; i < numToCreate; i++)
    {
        buffers[i] = builder.build();
    }

    return buffers;
}

void VolumeRenderer::addPreComputeMemoryBarriers(vk::CommandBuffer &cmdBuff, const star::common::FrameTracker &ft,
                                                 const bool getBuffersBackFromTransfer) const
{
    star::StarTextures::Texture *colorTex = m_renderingContext.recordDependentImage.get(
        m_offscreenRenderer->getRenderToColorImages()[ft.getCurrent().getFrameInFlightIndex()]);
    star::StarTextures::Texture *depthTex = m_renderingContext.recordDependentImage.get(
        m_offscreenRenderer->getRenderToDepthImages()[ft.getCurrent().getFrameInFlightIndex()]);

    const bool diffQueues = this->computeQueueFamilyIndex != this->graphicsQueueFamilyIndex;

    std::vector<vk::ImageMemoryBarrier2> prepareImages = std::vector<vk::ImageMemoryBarrier2>{
        vk::ImageMemoryBarrier2()
            .setImage(colorTex->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(diffQueues ? this->graphicsQueueFamilyIndex : vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(diffQueues ? this->computeQueueFamilyIndex : vk::QueueFamilyIgnored)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)),
        vk::ImageMemoryBarrier2()
            .setImage(depthTex->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(diffQueues ? this->graphicsQueueFamilyIndex : vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(diffQueues ? this->computeQueueFamilyIndex : vk::QueueFamilyIgnored)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)
                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth))};

    if (this->isFirstPass)
    {
        prepareImages.push_back(
            vk::ImageMemoryBarrier2()
                .setImage(this->computeWriteToImages[ft.getCurrent().getFrameInFlightIndex()]->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eUndefined)
                .setNewLayout(vk::ImageLayout::eGeneral)
                .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)));
    }
    else
    {
        prepareImages.push_back(
            vk::ImageMemoryBarrier2()
                .setImage(this->computeWriteToImages[ft.getCurrent().getFrameInFlightIndex()]->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setNewLayout(vk::ImageLayout::eGeneral)
                .setSrcQueueFamilyIndex(diffQueues ? this->graphicsQueueFamilyIndex : vk::QueueFamilyIgnored)
                .setDstQueueFamilyIndex(diffQueues ? this->computeQueueFamilyIndex : vk::QueueFamilyIgnored)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)));
    }

    std::vector<vk::BufferMemoryBarrier2> buffBarriers;
    if (getBuffersBackFromTransfer)
    {
        buffBarriers = getBufferBarriersFromTransferQueues(ft);
    }

    cmdBuff.pipelineBarrier2(
        vk::DependencyInfo().setImageMemoryBarriers(prepareImages).setBufferMemoryBarriers(buffBarriers));
}

inline static vk::BufferMemoryBarrier2 CreatePreBufferMemoryBarrier(const uint32_t &srcQueue, const uint32_t &dstQueue,
                                                                    vk::Buffer buffer) noexcept
{
    return vk::BufferMemoryBarrier2()
        .setBuffer(buffer)
        .setSize(vk::WholeSize)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setSrcQueueFamilyIndex(srcQueue)
        .setDstQueueFamilyIndex(dstQueue);
}

inline static vk::BufferMemoryBarrier2 CreatePostBufferMemoryBarrier(uint32_t srcQueue, uint32_t dstQueue,
                                                                     vk::Buffer buffer)
{
    return vk::BufferMemoryBarrier2()
        .setBuffer(buffer)
        .setSize(vk::WholeSize)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSrcQueueFamilyIndex(std::move(srcQueue))
        .setDstQueueFamilyIndex(std::move(dstQueue));
}

std::vector<vk::BufferMemoryBarrier2> VolumeRenderer::getBufferBarriersFromTransferQueues(
    const star::common::FrameTracker &ft) const
{
    const bool diffQueues = this->computeQueueFamilyIndex != this->transferQueueFamilyIndex;
    const uint32_t srcI = diffQueues ? this->transferQueueFamilyIndex : vk::QueueFamilyIgnored;
    const uint32_t dstI = diffQueues ? this->computeQueueFamilyIndex : vk::QueueFamilyIgnored;
    return {
        CreatePreBufferMemoryBarrier(
            srcI, dstI,
            this->computeRayAtCutoffDistanceBuffers[ft.getCurrent().getFrameInFlightIndex()].getVulkanBuffer()),
        CreatePreBufferMemoryBarrier(
            srcI, dstI, this->computeRayDistanceBuffers[ft.getCurrent().getFrameInFlightIndex()].getVulkanBuffer())};
}

std::vector<vk::BufferMemoryBarrier2> VolumeRenderer::getBufferBarriersToTransferQueues(
    const star::common::FrameTracker &ft) const
{
    const bool diffQueues = this->computeQueueFamilyIndex != this->transferQueueFamilyIndex;

    const uint32_t srcI = diffQueues ? this->computeQueueFamilyIndex : vk::QueueFamilyIgnored;
    const uint32_t dstI = diffQueues ? this->transferQueueFamilyIndex : vk::QueueFamilyIgnored;
    return {CreatePostBufferMemoryBarrier(
                srcI, dstI, this->computeRayDistanceBuffers[ft.getCurrent().getFrameInFlightIndex()].getVulkanBuffer()),
            CreatePostBufferMemoryBarrier(
                srcI, dstI,
                this->computeRayAtCutoffDistanceBuffers[ft.getCurrent().getFrameInFlightIndex()].getVulkanBuffer())};
}

void VolumeRenderer::addPostComputeMemoryBarriers(vk::CommandBuffer &cmdBuff, const star::common::FrameTracker &ft,
                                                  const bool giveBuffersToTransfer) const
{
    star::StarTextures::Texture *colorTex = m_renderingContext.recordDependentImage.get(
        m_offscreenRenderer->getRenderToColorImages()[ft.getCurrent().getFrameInFlightIndex()]);
    star::StarTextures::Texture *depthTex = m_renderingContext.recordDependentImage.get(
        m_offscreenRenderer->getRenderToDepthImages()[ft.getCurrent().getFrameInFlightIndex()]);

    const bool diffQueues = this->computeQueueFamilyIndex != this->graphicsQueueFamilyIndex;
    std::vector<vk::ImageMemoryBarrier2> imageBarriers;

    // give render to image back to graphics queue
    if (diffQueues)
    {
        imageBarriers = std::vector<vk::ImageMemoryBarrier2>{
            vk::ImageMemoryBarrier2()
                .setImage(colorTex->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eGeneral)
                .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
                .setDstAccessMask(vk::AccessFlagBits2::eNone)
                .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex)
                .setDstQueueFamilyIndex(this->graphicsQueueFamilyIndex)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)),
            vk::ImageMemoryBarrier2()
                .setImage(depthTex->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
                .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                 vk::PipelineStageFlagBits2::eLateFragmentTests)
                .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead)
                .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex)
                .setDstQueueFamilyIndex(this->graphicsQueueFamilyIndex)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)),
            vk::ImageMemoryBarrier2()
                .setImage(this->computeWriteToImages[ft.getCurrent().getFrameInFlightIndex()]->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eGeneral)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
                .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
                .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex)
                .setDstQueueFamilyIndex(this->graphicsQueueFamilyIndex)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1))};
    }

    std::vector<vk::BufferMemoryBarrier2> buffBarriers;

    if (giveBuffersToTransfer)
    {
        buffBarriers = getBufferBarriersToTransferQueues(ft);
    }
    cmdBuff.pipelineBarrier2(
        vk::DependencyInfo().setImageMemoryBarriers(imageBarriers).setBufferMemoryBarriers(buffBarriers));
}