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

static void GetTimelineSemaphoreInfo(const star::core::CommandBus &cmdBus, const star::common::FrameTracker &ft,
                                     const star::Handle &cmdBuff, vk::Semaphore &semaphore, uint64_t &value) noexcept
{
    auto cmd = star::command_order::GetPassInfo{cmdBuff};
    cmdBus.submit(cmd);

    semaphore = cmd.getReply().get().signaledSemaphore;
    value = cmd.getReply().get().toSignalValue;
}

static void GetTimelineSemaphoreInfo(const star::core::CommandBus &cmdBus, const star::common::FrameTracker &ft,
                                     const star::Handle &cmdBuff, vk::Semaphore &semaphore, uint64_t &value,
                                     uint64_t &currentValue) noexcept
{
    auto cmd = star::command_order::GetPassInfo{cmdBuff};
    cmdBus.submit(cmd);

    semaphore = cmd.getReply().get().signaledSemaphore;
    value = cmd.getReply().get().toSignalValue;
    currentValue = cmd.getReply().get().currentSignalValue;
}

static vk::SemaphoreSubmitInfo GetSignalSemaphoreInfo(const star::core::CommandBus &cmdBus,
                                                      const star::common::FrameTracker &ft,
                                                      const star::Handle &cmdBuff) noexcept
{
    vk::Semaphore signalSemaphore{VK_NULL_HANDLE};
    uint64_t value{0};
    uint64_t currentSignalValue{0};
    GetTimelineSemaphoreInfo(cmdBus, ft, cmdBuff, signalSemaphore, value);

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
      m_vdbFilePath(std::move(vdbFilePath)), aabbBounds(aabbBounds), camera(camera), volumeTexture(volumeTexture),
      m_distanceComputer()
{
}

void VolumeRenderer::init(star::core::device::DeviceContext &context)
{
    using registry = star::common::HandleTypeRegistry;

    m_device = context.getDevice().getVulkanDevice();
    m_cmdBus = &context.getCmdBus();

    m_timelineSemaphores = CreateSemaphores(context.getEventBus(), context.getFrameTracker());

    auto submitter = std::make_shared<star::wrappers::graphics::policies::SubmitDescriptorRequestsPolicy>(
        getDescriptorRequests(context.getFrameTracker().getSetup().getNumFramesInFlight()));

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
            &context.getManagerRenderResource(), context.getFrameTracker().getSetup().getNumFramesInFlight()})
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
        vk::Semaphore semaphore;
        uint64_t value;
        uint64_t currentValue;
        GetTimelineSemaphoreInfo(*m_cmdBus, frameTracker, m_commandBuffer, semaphore, value, currentValue);

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

void VolumeRenderer::recordCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                                    const uint64_t &frameIndex)
{
    auto tNeighbor = GetTransferNeighborInfo(*m_cmdBus, m_commandBuffer);

    // check if other dep was run this frame
    {
        const bool getFromTransfer = tNeighbor != nullptr && tNeighbor->wasProcessedOnLastFrame->at(
                                                                 frameTracker.getCurrent().getFrameInFlightIndex());
        addPreComputeMemoryBarriers(commandBuffer, frameTracker, getFromTransfer);
    }

    if (isReady)
    {
        m_renderingContext.pipeline->bind(commandBuffer);

        auto sets = m_staticShaderInfo->getDescriptors(frameTracker.getCurrent().getFrameInFlightIndex());
        {
            auto dynamicSets = m_dynamicShaderInfo->getDescriptors(frameTracker.getCurrent().getFrameInFlightIndex());
            sets.insert(sets.end(), dynamicSets.begin(), dynamicSets.end());
        }

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *this->computePipelineLayout, 0,
                                         static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

        commandBuffer.dispatch(this->workgroupSize.x, this->workgroupSize.y, 1);

        if (this->currentFogType == Fog::Type::sMarched)
            m_distanceComputer.recordCommandBuffer(commandBuffer, frameTracker, workgroupSize, currentFogType);
    }

    {
        const bool giveToTransfer = tNeighbor != nullptr && tNeighbor->isTriggeredThisFrame;
        addPostComputeMemoryBarriers(commandBuffer, frameTracker, giveToTransfer);
    }
}

vk::Semaphore VolumeRenderer::submitBuffer(star::StarCommandBuffer &buffer,
                                           const star::common::FrameTracker &frameTracker,
                                           std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                           std::vector<vk::Semaphore> dataSemaphores,
                                           std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                           std::vector<std::optional<uint64_t>> previousSignaledValues,
                                           star::StarQueue &queue)
{
    const size_t ii = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
    assert(m_cmdBus != nullptr);

    const auto cbInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(
        buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()));

    // get neighbors and look for the offscreen renderer
    vk::Semaphore offscreenSemaphore{VK_NULL_HANDLE};
    uint64_t offscreenSemaphoreSignalValue{0};
    vk::Semaphore mainRendererSemaphore{VK_NULL_HANDLE};
    uint64_t mainRendererPreviousValue{0};

    auto getCmd = star::command_order::GetPassInfo{m_commandBuffer};
    m_cmdBus->submit(getCmd);

    std::vector<vk::SemaphoreSubmitInfo> waitInfo;

    const star::command_order::get_pass_info::GatheredPassInfo &ele = getCmd.getReply().get();
    if (ele.edges != nullptr)
    {
        waitInfo.resize(ele.edges->size());

        for (size_t i{0}; i < ele.edges->size(); i++)
        {
            const auto &edge = ele.edges->at(i);

            vk::Semaphore semaphore{VK_NULL_HANDLE};
            uint64_t signalValue{0};
            if (edge.consumer == m_commandBuffer)
            {
                auto nCmd = star::command_order::GetPassInfo{edge.producer};
                m_cmdBus->submit(nCmd);

                semaphore = nCmd.getReply().get().signaledSemaphore;
                signalValue = nCmd.getReply().get().toSignalValue;
            }
            else
            {
                auto nCmd = star::command_order::GetPassInfo{edge.consumer};
                m_cmdBus->submit(nCmd);

                semaphore = nCmd.getReply().get().signaledSemaphore;
                signalValue = nCmd.getReply().get().currentSignalValue;
            }

            waitInfo[i] = vk::SemaphoreSubmitInfo()
                              .setSemaphore(semaphore)
                              .setValue(signalValue)
                              .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
        }
    }
    else
    {
        STAR_THROW("Unable to get binary semaphore from offscreen renderer");
    }

    assert(dataSemaphores.size() == dataWaitPoints.size());
    for (size_t i{0}; i < dataWaitPoints.size(); i++)
    {
        waitInfo.emplace_back(vk::SemaphoreSubmitInfo()
                                  .setSemaphore(dataSemaphores[i])
                                  .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
    }

    vk::Semaphore binarySemaphore{buffer.getCompleteSemaphores()[ii]};
    const vk::SemaphoreSubmitInfo signalInfo[1]{GetSignalSemaphoreInfo(*m_cmdBus, frameTracker, m_commandBuffer)};

    const auto submitInfo = vk::SubmitInfo2()
                                .setPWaitSemaphoreInfos(waitInfo.data())
                                .setWaitSemaphoreInfoCount(waitInfo.size())
                                .setCommandBufferInfos(cbInfo)
                                .setSignalSemaphoreInfos(signalInfo);

    queue.getVulkanQueue().submit2(submitInfo);

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

    m_timelineSemaphores = CreateSemaphores(context.getEventBus(), context.getFrameTracker());

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

    this->workgroupSize = CalculateWorkGroupSize(screensize);

    {
        const size_t n = static_cast<size_t>(context.getFrameTracker().getSetup().getNumFramesInFlight());
        this->computeWriteToImages = createComputeWriteToImages(context, screensize, n);
        this->computeRayDistanceBuffers =
            createComputeWriteToBuffers(context, screensize, sizeof(float), "RayDistanceBuffer", n);
        computeRayAtCutoffDistanceBuffers =
            createComputeWriteToBuffers(context, screensize, sizeof(uint32_t), "RayScissorBuffer", n);
    }

    m_fogController.prepRender(context, context.getFrameTracker().getSetup().getNumFramesInFlight());

    for (uint8_t i = 0; i < context.getFrameTracker().getSetup().getNumFramesInFlight(); i++)
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
        context.getFrameTracker().getSetup().getNumFramesInFlight());

    auto cmd = star::command_order::DeclarePass(m_commandBuffer, this->computeQueueFamilyIndex);
    context.begin().set(cmd).submit();

    for (size_t i = 0; i < static_cast<size_t>(context.getFrameTracker().getSetup().getNumFramesInFlight()); i++)
    {
        auto &ch = m_offscreenRenderer->getRenderToColorImages()[i];
        m_renderingContext.recordDependentImage.manualInsert(ch, &context.getImageManager().get(ch)->texture);
        auto &dh = m_offscreenRenderer->getRenderToDepthImages()[i];
        m_renderingContext.recordDependentImage.manualInsert(dh, &context.getImageManager().get(dh)->texture);
    }
}

void VolumeRenderer::cleanupRender(star::core::device::DeviceContext &context)
{
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
    vk::Semaphore dataSemaphore = VK_NULL_HANDLE;

    if (m_fogController.submitUpdateIfNeeded(context, frameInFlightIndex, dataSemaphore))
    {
        context.getManagerCommandBuffer()
            .m_manager.get(m_commandBuffer)
            .oneTimeWaitSemaphoreInfo.insert(m_fogController.getHandle(frameInFlightIndex), std::move(dataSemaphore),
                                             vk::PipelineStageFlagBits::eComputeShader);
        m_renderingContext.addBufferToRenderingContext(context, m_fogController.getHandle(frameInFlightIndex));
    }

    if (m_infoManagerGlobalCamera->willBeUpdatedThisFrame(
            context.getFrameTracker().getCurrent().getGlobalFrameCounter(),
            context.getFrameTracker().getCurrent().getFrameInFlightIndex()))
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
                               .setSharingMode(vk::SharingMode::eConcurrent)
                               .setPQueueFamilyIndices(indices.data())
                               .setQueueFamilyIndexCount(2)
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

void VolumeRenderer::addPreComputeMemoryBarriers(vk::CommandBuffer &cmdBuff, const star::common::FrameTracker &ft,
                                                 const bool getBuffersBackFromTransfer) const
{
    star::StarTextures::Texture *colorTex = m_renderingContext.recordDependentImage.get(
        m_offscreenRenderer->getRenderToColorImages()[ft.getCurrent().getFrameInFlightIndex()]);
    star::StarTextures::Texture *depthTex = m_renderingContext.recordDependentImage.get(
        m_offscreenRenderer->getRenderToDepthImages()[ft.getCurrent().getFrameInFlightIndex()]);

    const bool diffQueues = this->computeQueueFamilyIndex != this->graphicsQueueFamilyIndex;

    vk::ImageMemoryBarrier2 imageBarriers[3];
    uint8_t barrierCountImage{3};

    imageBarriers[0]
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
                                 .setAspectMask(vk::ImageAspectFlagBits::eColor));

    imageBarriers[1]
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
                                 .setAspectMask(vk::ImageAspectFlagBits::eDepth));

    if (this->isFirstPass)
    {
        imageBarriers[2]
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
                                     .setLayerCount(1));
    }
    else
    {
        imageBarriers[2]
            .setImage(this->computeWriteToImages[ft.getCurrent().getFrameInFlightIndex()]->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
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
                                     .setLayerCount(1));
    }

    vk::BufferMemoryBarrier2 bufferBarriers[4];
    uint32_t count{0};
    if (getBuffersBackFromTransfer)
    {
        auto transferBarriers = getBufferBarriersFromTransferQueues(ft);
        bufferBarriers[0] = std::move(transferBarriers[0]);
        bufferBarriers[1] = std::move(transferBarriers[1]);
        count = 2;
    }

    if (m_infoManagerGlobalCamera->willBeUpdatedThisFrame(ft.getCurrent().getGlobalFrameCounter(),
                                                          ft.getCurrent().getFrameInFlightIndex()))
    {
        auto buffer = m_renderingContext.bufferTransferRecords.get(
            m_infoManagerGlobalCamera->getHandle(ft.getCurrent().getFrameInFlightIndex()));
        bufferBarriers[count]
            .setBuffer(std::move(buffer))
            .setSize(vk::WholeSize)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
        count++;
    }

    if (m_fogController.willBeUpdatedThisFrame(ft.getCurrent().getGlobalFrameCounter(),
                                               ft.getCurrent().getFrameInFlightIndex()))
    {
        auto buffer = m_renderingContext.bufferTransferRecords.get(
            m_fogController.getHandle(ft.getCurrent().getFrameInFlightIndex()));
        bufferBarriers[count]
            .setBuffer(std::move(buffer))
            .setSize(vk::WholeSize)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
        count++;
    }

    cmdBuff.pipelineBarrier2(vk::DependencyInfo()
                                 .setPImageMemoryBarriers(imageBarriers)
                                 .setImageMemoryBarrierCount(barrierCountImage)
                                 .setPBufferMemoryBarriers(bufferBarriers)
                                 .setBufferMemoryBarrierCount(count));
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
        .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSrcQueueFamilyIndex(std::move(srcQueue))
        .setDstQueueFamilyIndex(std::move(dstQueue));
}

std::array<vk::BufferMemoryBarrier2, 2> VolumeRenderer::getBufferBarriersFromTransferQueues(
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

std::array<vk::BufferMemoryBarrier2, 2> VolumeRenderer::getBufferBarriersToTransferQueues(
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
    vk::ImageMemoryBarrier2 imageBarriers[3];
    uint8_t barrierCountImage{0};

    // give render to image back to graphics queue
    if (diffQueues)
    {
        imageBarriers[0]
            .setImage(colorTex->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
            .setDstAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex)
            .setDstQueueFamilyIndex(this->graphicsQueueFamilyIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1));
        imageBarriers[1]
            .setImage(depthTex->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
            .setDstAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex)
            .setDstQueueFamilyIndex(this->graphicsQueueFamilyIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1));
        imageBarriers[2]
            .setImage(computeWriteToImages[ft.getCurrent().getFrameInFlightIndex()]->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
            .setDstAccessMask(vk::AccessFlagBits2::eNone)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(vk::RemainingMipLevels)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(vk::RemainingArrayLayers));

        barrierCountImage = 3;
    };

    vk::BufferMemoryBarrier2 bufferBarriers[5];
    uint8_t barrierCountBuffer{0};
    if (giveBuffersToTransfer)
    {
        auto transferBarriers = getBufferBarriersToTransferQueues(ft);
        bufferBarriers[0] = transferBarriers[0];
        bufferBarriers[1] = transferBarriers[1];
        barrierCountBuffer = 2;
    }

    auto dependencyInfo = vk::DependencyInfo()
                              .setPImageMemoryBarriers(imageBarriers)
                              .setImageMemoryBarrierCount(barrierCountImage)
                              .setPBufferMemoryBarriers(bufferBarriers)
                              .setBufferMemoryBarrierCount(barrierCountBuffer);
    cmdBuff.pipelineBarrier2(dependencyInfo);
}