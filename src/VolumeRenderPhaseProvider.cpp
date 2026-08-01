#include "VolumeRenderPhaseProvider.hpp"

#include "renderer/VolumeRenderPhase.hpp"

#include <starlight/core/renderer/RenderPhase.hpp>

#include "AABBTransfer.hpp"
#include "Allocator.hpp"
#include "CameraInfo.hpp"
#include "ConfigFile.hpp"
#include "FogData.hpp"
#include "ManagerRenderResource.hpp"
#include "RandomValueTexture.hpp"
#include "VDBTransfer.hpp"
#include "VolumeDirectoryProcessor.hpp"
#include "core/device/managers/DescriptorPool.hpp"
#include "event/EnginePhaseComplete.hpp"
#include "render_system/fog/util/CreateBuffers.hpp"
#include "renderer/volume/ContainerRenderResourceData.hpp"
#include "renderer/volume/DescriptorBuilder.hpp"
#include "starlight/core/waiter/one_shot/CreateDescriptorsOnEventPolicy.hpp"
#include "wrappers/graphics/policies/SubmitDescriptorRequestsPolicy.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/core/device/system/event/ManagerRequest.hpp>

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

#include <star_common/FrameTracker.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <cassert>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

static std::vector<std::shared_ptr<star::StarTextures::Texture>> CreateComputeWriteToImages(
    star::core::device::DeviceContext &context, const vk::Extent2D &screenSize, const size_t &numToCreate,
    const std::vector<uint32_t> allQueueFamilyInds)
{
    auto textures = std::vector<std::shared_ptr<star::StarTextures::Texture>>(numToCreate);

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

VolumeRenderPhaseProvider::VolumeRenderPhaseProvider(
    star::ManagerController::RenderResource::Buffer *instanceManagerInfo,
    star::ManagerController::RenderResource::Buffer *instanceNormalInfo,
    std::shared_ptr<star::core::renderer::FrameData> frameData, star::Handle offscreenPhaseHandle,
    std::string vdbFilePath, std::shared_ptr<star::StarCamera> camera, const std::array<glm::vec4, 2> &aabbBounds,
    bool enableCutoffHighlighting)
    : m_infoManagerInstanceModel(instanceManagerInfo), m_infoManagerInstanceNormal(instanceNormalInfo),
      m_frameData(std::move(frameData)), m_offscreenPhaseHandle(std::move(offscreenPhaseHandle)), m_vdbFilePath(std::move(vdbFilePath)),
      m_camera(std::move(camera)), m_aabbBounds(aabbBounds), m_enableCutoffHighlighting(enableCutoffHighlighting)
{
}

std::unique_ptr<star::core::renderer::RenderPhase> VolumeRenderPhaseProvider::build(
    star::core::device::DeviceContext &c, star::core::renderer::RenderPhaseRegistry &phases)
{
    using registry = star::common::HandleTypeRegistry;
    const vk::Extent2D screensize = c.getEngineResolution();
    const uint8_t numFramesInFlight = c.frameTracker().getSetup().getNumFramesInFlight();
    const size_t n = static_cast<size_t>(numFramesInFlight);

    auto phase = std::make_unique<VolumeRenderPhase>(m_enableCutoffHighlighting);

    // --- init() equivalent ---
    phase->m_device = c.getDevice().getVulkanDevice();
    phase->m_cmdBus = &c.getCmdBus();
    phase->m_frameData = m_frameData;
    auto *offscreenPhase = phases.getPhase(m_offscreenPhaseHandle);
    assert(offscreenPhase != nullptr &&
           "offscreen render phase must be built before the volume render phase");
    phase->m_offscreenPhase = offscreenPhase;

    phase->m_timelineSemaphores = CreateSemaphores(c.getEventBus(), c.frameTracker());

    auto submitter = std::make_shared<star::wrappers::graphics::policies::SubmitDescriptorRequestsPolicy>(
        VolumeRenderPhase::getDescriptorRequests(numFramesInFlight));
    submitter->init(c.getEventBus());

    if (!registry::instance().contains(star::event::EnginePhaseComplete::GetUniqueTypeName()))
    {
        registry::instance().registerType(star::event::EnginePhaseComplete::GetUniqueTypeName());
    }

    renderer::volume::ContainerRenderResourceData pipelineData{
        .inputs{.fogController = &phase->m_fogController,
                .aabbInfoBuffers = &phase->aabbInfoBuffers,
                .offscreenRenderToColors = &offscreenPhase->getRenderToColorImages(),
                .offscreenRenderToDepths = &offscreenPhase->getRenderToDepthImages(),
                .instanceManagerInfo = m_infoManagerInstanceModel,
                .instanceNormalInfo = m_infoManagerInstanceNormal,
                .globalInfoBuffers = phase->m_frameData->controllerAt(0).get(),
                .globalLightList = phase->m_frameData->controllerAt(1).get(),
                .globalLightInfo = phase->m_frameData->controllerAt(2).get(),
                .cameraShaderInfo = &phase->cameraShaderInfo,
                .vdbInfoFog = &phase->vdbInfoFog,
                .randomValueTexture = &phase->randomValueTexture,
                .activeRayStorageBuffers = &phase->m_activeRayStorage},
        .outputs{.computeWriteToImages = &phase->computeWriteToImages,
                 .computeRayDistBuffers = &phase->computeRayDistanceBuffers,
                 .computeRayAtCutoffBuffer = &phase->computeRayAtCutoffDistanceBuffers}};

    star::core::waiter::one_shot::CreateDescriptorsOnEventPolicy<DescriptorBuilder>::Builder(c.getEventBus())
        .setEventType(
            registry::instance().getTypeGuaranteedExist(star::event::EnginePhaseComplete::GetUniqueTypeName()))
        .setPolicy(DescriptorBuilder{&c.getDeviceID(),
                                     pipelineData,
                                     &phase->m_staticShaderInfo,
                                     &phase->m_dynamicShaderInfo,
                                     &phase->marchedHomogenousPipeline,
                                     &phase->nanoVDBPipeline_hitBoundingBox,
                                     &phase->nanoVDBPipeline_surface,
                                     &phase->marchedPipeline,
                                     &phase->linearPipeline,
                                     &phase->expPipeline,
                                     &phase->computePipelineLayout,
                                     &phase->m_initPipe,
                                     &phase->m_pipeInfo.initPipeline,
                                     &phase->m_indirectDispatchPipe,
                                     &phase->m_pipeInfo.indirectDispatchPipeline,
                                     &c.getDevice(),
                                     &c.getGraphicsManagers(),
                                     &c.getManagerRenderResource(),
                                     &c.getEventBus(),
                                     numFramesInFlight})
        .buildShared();

    phase->m_distanceComputer.prepRender(c, pipelineData, &phase->m_staticShaderInfo);

    // --- prepRender rest ---
    const uint32_t computeQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(c.getEventBus(), c.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tcompute)
            ->getParentQueueFamilyIndex();

    const uint32_t graphicsQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(c.getEventBus(), c.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();
    phase->transferQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(c.getEventBus(), c.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Ttransfer)
            ->getParentQueueFamilyIndex();

    phase->m_timelineSemaphores = CreateSemaphores(c.getEventBus(), c.frameTracker());

    phase->cameraShaderInfo = star::ManagerRenderResource::addRequest(
        c.getDeviceID(),
        std::make_unique<CameraInfo>(
            m_camera, computeQueueFamilyIndex,
            c.getDevice().getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment),
        nullptr, true, &phase->transferQueueFamilyIndex);

    // need to find a way to tell what type the volume is...
    // dragon is level set
    const auto tmpDir = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::tmp_directory));
    VolumeDirectoryProcessor processor(m_vdbFilePath, tmpDir);
    processor.init();

    const auto &frontPath = processor.getProcessedFiles().front().getDataFilePath();

    phase->vdbInfoFog = star::ManagerRenderResource::addRequest(
        c.getDeviceID(),
        std::make_unique<VDBTransfer>(
            computeQueueFamilyIndex,
            std::make_unique<FogData>(frontPath.string(), openvdb::GridClass::GRID_FOG_VOLUME)),
        nullptr, true);

    phase->randomValueTexture = star::ManagerRenderResource::addRequest(
        c.getDeviceID(),
        std::make_unique<RandomValueTexture>(screensize.width, screensize.height, computeQueueFamilyIndex,
                                             c.getDevice().getPhysicalDevice().getProperties()));

    {
        std::vector<uint32_t> inds{graphicsQueueFamilyIndex};
        if (graphicsQueueFamilyIndex != computeQueueFamilyIndex)
        {
            inds.push_back(computeQueueFamilyIndex);
            inds.push_back(phase->transferQueueFamilyIndex);
        }

        phase->computeWriteToImages = CreateComputeWriteToImages(c, screensize, n, inds);
        phase->computeRayDistanceBuffers =
            VolumeRenderPhase::createComputeWriteToBuffers(c, screensize, sizeof(float), "RayDistanceBuffer", n);
        phase->computeRayAtCutoffDistanceBuffers =
            VolumeRenderPhase::createComputeWriteToBuffers(c, screensize, sizeof(uint32_t), "RayScissorBuffer", n);
    }

    phase->m_fogController.prepRender(c, numFramesInFlight);

    phase->m_activeRayStorage.resize(numFramesInFlight);
    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        phase->aabbInfoBuffers.emplace_back(star::ManagerRenderResource::addRequest(
            c.getDeviceID(), std::make_unique<AABBTransfer>(graphicsQueueFamilyIndex, m_aabbBounds)));

        const std::string cstr = std::to_string(i);
        phase->m_activeRayStorage[i] =
            render_system::fog::CreateActiveRayStorageBuffer(c, "RMEM_" + cstr, c.getEngineResolution());
    }

    phase->m_commandBuffer = c.getManagerCommandBuffer().submit(
        star::core::device::manager::ManagerCommandBuffer::Request{
            .recordBufferCallback = std::bind(&VolumeRenderPhase::recordCommandBuffer, phase.get(),
                                              std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            .order = star::Command_Buffer_Order::before_render_pass,
            .orderIndex = star::Command_Buffer_Order_Index::second,
            .type = star::Queue_Type::Tcompute,
            .waitStage = vk::PipelineStageFlagBits::eAllCommands,
            .willBeSubmittedEachFrame = true,
            .recordOnce = false,
            .overrideBufferSubmissionCallback = phase->getSubmissionOverride()},
        numFramesInFlight);

    auto cmd = star::command_order::DeclarePass(phase->m_commandBuffer, computeQueueFamilyIndex);
    c.begin().set(cmd).submit();

    for (size_t i = 0; i < n; i++)
    {
        auto &ch = offscreenPhase->getRenderToColorImages()[i];
        phase->m_renderingContext.recordDependentImage.manualInsert(ch, &c.getImageManager().get(ch)->texture);
        auto &dh = offscreenPhase->getRenderToDepthImages()[i];
        phase->m_renderingContext.recordDependentImage.manualInsert(dh, &c.getImageManager().get(dh)->texture);
    }

    phase->m_chunkHandler.prepRender(c, phase->m_commandBuffer, phase->isReady);

    // apply the initial fog config the application threaded in before build()
    phase->setFogInfo(std::move(m_fogInfo));
    phase->setFogType(m_fogType);
    if (m_transferNeighborHandle.has_value())
    {
        phase->setTransferNeighborHandle(m_transferNeighborHandle.value());
    }

    return phase;
}