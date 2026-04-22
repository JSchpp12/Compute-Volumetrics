#include "renderer/VolumeRenderer.hpp"

#include "AABBTransfer.hpp"
#include "CameraInfo.hpp"
#include "ConfigFile.hpp"
#include "FogControlInfo.hpp"
#include "FogData.hpp"
#include "ManagerRenderResource.hpp"
#include "RandomValueTexture.hpp"
#include "VDBTransfer.hpp"
#include "VolumeDirectoryProcessor.hpp"
#include "core/device/managers/DescriptorPool.hpp"
#include "event/EnginePhaseComplete.hpp"

#include "renderer/volume/ContainerRenderResourceData.hpp"
#include "renderer/volume/DescriptorBuilder.hpp"
#include "starlight/core/waiter/one_shot/CreateDescriptorsOnEventPolicy.hpp"
#include "wrappers/graphics/policies/SubmitDescriptorRequestsPolicy.hpp"

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

#include <star_common/HandleTypeRegistry.hpp>

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

static std::unique_ptr<star::command_order::get_pass_info::GatheredPassInfo> GetTransferNeighborInfo(
    const star::core::CommandBus &cmdBus, const star::Handle &commandBuffer)
{
    std::unique_ptr<star::command_order::get_pass_info::GatheredPassInfo> transferNeighborInfo = nullptr;

    auto getCmd = star::command_order::GetPassInfo(commandBuffer);
    cmdBus.submit(getCmd);

    // command deps
    const star::command_order::get_pass_info::GatheredPassInfo &ele = getCmd.getReply().get();

    if (ele.edges != nullptr)
    {
        const auto &edge = ele.edges->front();
        const star::Handle f = edge.producer == commandBuffer ? edge.consumer : edge.producer;

        auto nGetCmd = star::command_order::GetPassInfo(f);
        cmdBus.submit(nGetCmd);

        transferNeighborInfo =
            std::make_unique<star::command_order::get_pass_info::GatheredPassInfo>(nGetCmd.getReply().get());
    }

    return transferNeighborInfo;
}

static std::tuple<vk::Semaphore, uint64_t, uint64_t> GetTimelineSemaphoreInfo(const star::core::CommandBus &cmdBus,
                                                                              const star::common::FrameTracker &ft,
                                                                              const star::Handle &cmdBuff) noexcept
{
    auto cmd = star::command_order::GetPassInfo{cmdBuff};
    cmdBus.submit(cmd);

    auto &r = cmd.getReply().get();
    return std::make_tuple(r.signaledSemaphore, r.toSignalValue, r.currentSignalValue);
}

static vk::SemaphoreSubmitInfo GetSignalSemaphoreInfo(const star::core::CommandBus &cmdBus,
                                                      const star::common::FrameTracker &ft,
                                                      const star::Handle &cmdBuff) noexcept
{
    auto [signalSemaphore, value, currentSignalValue] = GetTimelineSemaphoreInfo(cmdBus, ft, cmdBuff);

    return vk::SemaphoreSubmitInfo()
        .setSemaphore(std::move(signalSemaphore))
        .setValue(std::move(value))
        .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
}

VolumeRenderer::VolumeRenderer(star::core::device::DeviceContext &context,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceManagerInfo,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceNormalInfo,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalInfoBuffers,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneLightInfoBuffers,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneLightList,
                               OffscreenRenderer *offscreenRenderer, std::string vdbFilePath,
                               const std::shared_ptr<star::StarCamera> camera,
                               const std::array<glm::vec4, 2> &aabbBounds)
    : m_infoManagerInstanceModel(instanceManagerInfo), m_infoManagerInstanceNormal(instanceNormalInfo),
      m_infoManagerGlobalCamera(globalInfoBuffers), m_infoManagerSceneLightInfo(sceneLightInfoBuffers),
      m_infoManagerSceneLightList(sceneLightList), m_offscreenRenderer(offscreenRenderer),
      m_vdbFilePath(std::move(vdbFilePath)), aabbBounds(aabbBounds), camera(camera), volumeTexture(),
      m_distanceComputer(), m_chunkHandler({32,32})
{
}

void VolumeRenderer::init(star::core::device::DeviceContext &context)
{
    using registry = star::common::HandleTypeRegistry;

    m_device = context.getDevice().getVulkanDevice();
    m_cmdBus = &context.getCmdBus();

    m_timelineSemaphores = CreateSemaphores(context.getEventBus(), context.frameTracker());

    auto submitter = std::make_shared<star::wrappers::graphics::policies::SubmitDescriptorRequestsPolicy>(
        getDescriptorRequests(context.frameTracker().getSetup().getNumFramesInFlight()));

    submitter->init(context.getEventBus());

    if (!registry::instance().contains(star::event::EnginePhaseComplete::GetUniqueTypeName()))
    {
        registry::instance().registerType(star::event::EnginePhaseComplete::GetUniqueTypeName());
    }

    renderer::volume::ContainerRenderResourceData pipelineData{
        .inputs{.fogController = &m_fogController,
                .aabbInfoBuffers = &aabbInfoBuffers,
                .offscreenRenderToColors = &m_offscreenRenderer->getRenderToColorImages(),
                .offscreenRenderToDepths = &m_offscreenRenderer->getRenderToDepthImages(),
                .instanceManagerInfo = m_infoManagerInstanceModel,
                .instanceNormalInfo = m_infoManagerInstanceNormal,
                .globalInfoBuffers = m_infoManagerGlobalCamera,
                .globalLightList = m_infoManagerSceneLightList,
                .globalLightInfo = m_infoManagerSceneLightInfo,
                .cameraShaderInfo = &cameraShaderInfo,
                .vdbInfoSDF = &vdbInfoSDF,
                .vdbInfoFog = &vdbInfoFog,
                .randomValueTexture = &randomValueTexture},
        .outputs{.computeWriteToImages = &computeWriteToImages,
                 .computeRayDistBuffers = &computeRayDistanceBuffers,
                 .computeRayAtCutoffBuffer = &computeRayAtCutoffDistanceBuffers}};

    star::core::waiter::one_shot::CreateDescriptorsOnEventPolicy<DescriptorBuilder>::Builder(context.getEventBus())
        .setEventType(
            registry::instance().getTypeGuaranteedExist(star::event::EnginePhaseComplete::GetUniqueTypeName()))
        .setPolicy(DescriptorBuilder{
            &context.getDeviceID(), pipelineData, &m_staticShaderInfo, &m_dynamicShaderInfo, &marchedHomogenousPipeline,
            &nanoVDBPipeline_hitBoundingBox, &nanoVDBPipeline_surface, &marchedPipeline, &linearPipeline, &expPipeline,
            &computePipelineLayout, &context.getDevice(), &context.getGraphicsManagers(),
            &context.getManagerRenderResource(), context.frameTracker().getSetup().getNumFramesInFlight()})
        .buildShared();

    m_distanceComputer.prepRender(context, pipelineData, &this->m_staticShaderInfo);
}

bool VolumeRenderer::isRenderReady(const star::core::device::DeviceContext &context)
{
    if (isReady)
    {
        return true;
    }

    if (context.getPipelineManager().get(marchedPipeline)->isReady() &&
        context.getPipelineManager().get(linearPipeline)->isReady() &&
        context.getPipelineManager().get(expPipeline)->isReady() &&
        context.getPipelineManager().get(nanoVDBPipeline_hitBoundingBox)->isReady() &&
        context.getPipelineManager().get(nanoVDBPipeline_surface)->isReady() &&
        context.getPipelineManager().get(marchedHomogenousPipeline)->isReady() && m_distanceComputer.isReady(context))
    {
        isReady = true;
    }

    return isReady;
}

void VolumeRenderer::frameUpdate(star::core::device::DeviceContext &context, uint8_t frameInFlightIndex)
{
    updateRenderingContext(context, frameInFlightIndex);

    if (isRenderReady(context))
    {
        updateDependentData(context, frameInFlightIndex);
        gatherDependentExternalDataOrderingInfo(context, frameInFlightIndex);

        m_distanceComputer.frameUpdate(context);
    }
}

void VolumeRenderer::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                         const star::common::FrameTracker &frameTracker, const uint64_t &frameIndex)
{
    {
        auto [semaphore, value, currentValue] = GetTimelineSemaphoreInfo(*m_cmdBus, frameTracker, m_commandBuffer);

        if (frameTracker.getCurrent().getNumTimesFrameProcessed() == currentValue)
        {
            const auto result = m_device.waitSemaphores(
                vk::SemaphoreWaitInfo().setValues(currentValue).setSemaphores(semaphore), UINT64_MAX);

            if (result != vk::Result::eSuccess)
            {
                STAR_THROW("Failed to wait for timeline semaphores");
            }
        }
    }

    commandBuffer.begin(frameTracker.getCurrent().getFrameInFlightIndex());

    recordCommands(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()), frameTracker, frameIndex);

    commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()).end();
}

void VolumeRenderer::recordCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &ft,
                                    const uint64_t &frameIndex)
{
    auto tNeighbor = GetTransferNeighborInfo(*m_cmdBus, m_commandBuffer);

    render_system::fog::PassInfo tInfo{
        .globalCameraBuffer = m_infoManagerGlobalCamera->willBeUpdatedThisFrame(ft.getCurrent().getGlobalFrameCounter(),
                                                                                ft.getCurrent().getFrameInFlightIndex())
                                  ? std::make_optional(m_renderingContext.bufferTransferRecords.get(
                                        m_infoManagerGlobalCamera->getHandle(ft.getCurrent().getFrameInFlightIndex())))
                                  : std::nullopt,
        .fogControllerBuffer = m_fogController.willBeUpdatedThisFrame(ft.getCurrent().getGlobalFrameCounter(),
                                                                      ft.getCurrent().getFrameInFlightIndex())
                                   ? std::make_optional(m_renderingContext.bufferTransferRecords.get(
                                         m_fogController.getHandle(ft.getCurrent().getFrameInFlightIndex())))
                                   : std::nullopt,
        .terrainPassInfo =
            {.renderToColor =
                 m_renderingContext.recordDependentImage
                     .get(m_offscreenRenderer->getRenderToColorImages()[ft.getCurrent().getFrameInFlightIndex()])
                     ->getVulkanImage(),
             .renderToDepth =
                 m_renderingContext.recordDependentImage
                     .get(m_offscreenRenderer->getRenderToDepthImages()[ft.getCurrent().getFrameInFlightIndex()])
                     ->getVulkanImage()},
        .computeWriteToImage = computeWriteToImages[ft.getCurrent().getFrameInFlightIndex()]->getVulkanImage(),
        .computeRayAtCutoffDistance =
            computeRayAtCutoffDistanceBuffers[ft.getCurrent().getFrameInFlightIndex()].getVulkanBuffer(),
        .computeRayDistance = computeRayDistanceBuffers[ft.getCurrent().getFrameInFlightIndex()].getVulkanBuffer(),
        .transferWasRunLast = tNeighbor != nullptr
                                  ? tNeighbor->wasProcessedOnLastFrame->at(ft.getCurrent().getFrameInFlightIndex())
                                  : false,
        .transferWillBeRunThisFrame = tNeighbor != nullptr ? tNeighbor->isTriggeredThisFrame : false};

    render_system::fog::PassPipelineInfo pipeInfo{
        .colorPipe = {.layout = *this->computePipelineLayout,
                      .pipeline = m_renderingContext.pipeline->getVulkanPipeline()},
        .distancePipe = {.layout = m_distanceComputer.getLayout(), .pipeline = m_distanceComputer.getPipeline()},
        .staticShaderInfo = this->m_staticShaderInfo.get(),
        .colorOnlyShaderInfo = this->m_dynamicShaderInfo.get(),
        .distanceOnlyShaderInfo = m_distanceComputer.getDynamicShaderInfo()};

    vk::Semaphore workingSemaphore{VK_NULL_HANDLE};
    {
        uint64_t previousSignaledValue{0};
        auto gCmd = star::command_order::GetPassInfo{m_commandBuffer};
        m_cmdBus->submit(gCmd);
        const auto &r = gCmd.getReply().get();
        workingSemaphore = r.signaledSemaphore;
        previousSignaledValue = r.currentSignalValue;

        auto wait = vk::SemaphoreWaitInfo()
                        .setSemaphoreCount(1)
                        .setPSemaphores(&workingSemaphore)
                        .setPValues(&previousSignaledValue)
                        .setSemaphoreCount(1);

        try
        {
            const auto waitResult = m_device.waitSemaphores(wait, UINT64_MAX);
        }
        catch (const std::runtime_error &e)
        {
            std::ostringstream oss;
            oss << "Failed to wait for seamphores with error: " << e.what();
            STAR_THROW(oss.str());
        }
    }

    m_chunkHandler.recordAllChunks(ft, tInfo, pipeInfo, this->currentFogType);
}

uint64_t VolumeRenderer::getTimelineSignalValue(const star::common::FrameTracker &ft) const
{
    return m_chunkHandler.getTimelineDoneSignalValue(ft);
}

vk::Semaphore VolumeRenderer::submitBuffer(star::StarCommandBuffer &buffer,
                                           const star::common::FrameTracker &frameTracker,
                                           std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                           std::vector<vk::Semaphore> dataSemaphores,
                                           std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                           std::vector<std::optional<uint64_t>> previousSignaledValues,
                                           star::StarQueue &queue)
{

    m_chunkHandler.submitAllChunks(frameTracker, std::move(dataSemaphores), std::move(dataWaitPoints),
                                   std::move(previousSignaledValues), queue, m_commandBuffer);

    return vk::Semaphore();
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

void VolumeRenderer::prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &screensize)
{
    init(context);

    m_timelineSemaphores = CreateSemaphores(context.getEventBus(), context.frameTracker());

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
    VolumeDirectoryProcessor processor(m_vdbFilePath, tmpDir);
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

    {
        const size_t n = static_cast<size_t>(context.frameTracker().getSetup().getNumFramesInFlight());
        this->computeWriteToImages = createComputeWriteToImages(context, screensize, n);
        this->computeRayDistanceBuffers =
            createComputeWriteToBuffers(context, screensize, sizeof(float), "RayDistanceBuffer", n);
        computeRayAtCutoffDistanceBuffers =
            createComputeWriteToBuffers(context, screensize, sizeof(uint32_t), "RayScissorBuffer", n);
    }

    m_fogController.prepRender(context, context.frameTracker().getSetup().getNumFramesInFlight());

    for (uint8_t i = 0; i < context.frameTracker().getSetup().getNumFramesInFlight(); i++)
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
            .waitStage = vk::PipelineStageFlagBits::eAllCommands,
            .willBeSubmittedEachFrame = true,
            .recordOnce = false,
            .overrideBufferSubmissionCallback =
                std::bind(&VolumeRenderer::submitBuffer, this, std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6,
                          std::placeholders::_7)},
        context.frameTracker().getSetup().getNumFramesInFlight());

    auto cmd = star::command_order::DeclarePass(m_commandBuffer, this->computeQueueFamilyIndex);
    context.begin().set(cmd).submit();

    const size_t nf = static_cast<size_t>(context.frameTracker().getSetup().getNumFramesInFlight());
    for (size_t i = 0; i < nf; i++)
    {
        auto &ch = m_offscreenRenderer->getRenderToColorImages()[i];
        m_renderingContext.recordDependentImage.manualInsert(ch, &context.getImageManager().get(ch)->texture);
        auto &dh = m_offscreenRenderer->getRenderToDepthImages()[i];
        m_renderingContext.recordDependentImage.manualInsert(dh, &context.getImageManager().get(dh)->texture);
    }

    m_chunkHandler.prepRender(context, m_commandBuffer, isReady);
}

void VolumeRenderer::cleanupRender(star::core::device::DeviceContext &context)
{
    m_chunkHandler.cleanupRender(context);

    m_distanceComputer.cleanupRender(context);

    m_staticShaderInfo->cleanupRender(context.getDevice());
    m_dynamicShaderInfo->cleanupRender(context.getDevice());

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

std::vector<std::pair<vk::DescriptorType, const uint32_t>> VolumeRenderer::getDescriptorRequests(
    const int &numFramesInFlight)
{
    return std::vector<std::pair<vk::DescriptorType, const uint32_t>>{
        std::make_pair(vk::DescriptorType::eStorageImage, 1 + (3 * numFramesInFlight * 50)),
        std::make_pair(vk::DescriptorType::eUniformBuffer, 1 + (4 * numFramesInFlight * 50)),
        std::make_pair(vk::DescriptorType::eStorageBuffer, 6 * numFramesInFlight),
        std::make_pair(vk::DescriptorType::eCombinedImageSampler, 805 * numFramesInFlight)};
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
    const size_t fi = static_cast<size_t>(context.frameTracker().getCurrent().getFrameInFlightIndex());

    vk::Semaphore dataSemaphore = VK_NULL_HANDLE;
    {
        star::core::graphics::GPUWorkSyncInfo transferWaitOnLastCompute;
        auto cmd = star::command_order::GetPassInfo{m_commandBuffer};
        context.getCmdBus().submit(cmd);
        transferWaitOnLastCompute.workWaitOn.signalValue = cmd.getReply().get().currentSignalValue;
        transferWaitOnLastCompute.workWaitOn.semaphore = cmd.getReply().get().signaledSemaphore;

        if (m_fogController.submitUpdateIfNeeded(context, frameInFlightIndex, dataSemaphore, transferWaitOnLastCompute))
        {
            context.getManagerCommandBuffer()
                .m_manager.get(m_commandBuffer)
                .oneTimeWaitSemaphoreInfo.insert(m_fogController.getHandle(frameInFlightIndex),
                                                 std::move(dataSemaphore), vk::PipelineStageFlagBits::eComputeShader);
            m_renderingContext.addBufferToRenderingContext(context, m_fogController.getHandle(frameInFlightIndex));
        }
    }

    if (m_infoManagerGlobalCamera->willBeUpdatedThisFrame(context.frameTracker().getCurrent().getGlobalFrameCounter(),
                                                          context.frameTracker().getCurrent().getFrameInFlightIndex()))
    {
        m_renderingContext.addBufferToRenderingContext(context,
                                                       m_infoManagerGlobalCamera->getHandle(frameInFlightIndex));
    }
}

void VolumeRenderer::updateRenderingContext(star::core::device::DeviceContext &context,
                                            const uint8_t &frameInFlightIndex)
{
    switch (this->currentFogType)
    {
    case (Fog::Type::sMarched):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->marchedPipeline)->request.pipeline;
        break;
    case (Fog::Type::sLinear):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->linearPipeline)->request.pipeline;
        break;
    case (Fog::Type::sExponential):
        m_renderingContext.pipeline = &context.getPipelineManager().get(this->expPipeline)->request.pipeline;
        break;
    case (Fog::Type::sMarchedHomogenous):
        m_renderingContext.pipeline =
            &context.getPipelineManager().get(this->marchedHomogenousPipeline)->request.pipeline;
        break;
    case (Fog::Type::sNanoBoundingBox):
        m_renderingContext.pipeline =
            &context.getPipelineManager().get(this->nanoVDBPipeline_hitBoundingBox)->request.pipeline;
        break;
    case (Fog::Type::sNanoSurface):
        m_renderingContext.pipeline =
            &context.getPipelineManager().get(this->nanoVDBPipeline_surface)->request.pipeline;
        break;
    default:
        throw std::runtime_error("Unsupported type");
    }
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
                               .setSharingMode(vk::SharingMode::eExclusive)
                               .setArrayLayers(1)
                               .setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled)
                               .setImageType(vk::ImageType::e2D)
                               .setMipLevels(1)
                               .setTiling(vk::ImageTiling::eOptimal)
                               .setInitialLayout(vk::ImageLayout::eUndefined)
                               .setSamples(vk::SampleCountFlagBits::e1),
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
