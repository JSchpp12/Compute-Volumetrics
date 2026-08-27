#include "VolumeRenderPhaseProvider.hpp"

#include "renderer/VolumeRenderPhase.hpp"

#include "AABBTransfer.hpp"
#include "Allocator.hpp"
#include "CameraInfo.hpp"
#include "ConfigFile.hpp"
#include "DataRoles.hpp"
#include "FogData.hpp"
#include "ManagerRenderResource.hpp"
#include "RandomValueTexture.hpp"
#include "VDBTransfer.hpp"
#include "VolumeDirectoryProcessor.hpp"
#include "core/device/managers/DescriptorPool.hpp"
#include "core/device/managers/Image.hpp"
#include "core/renderer/DescriptorRecipe.hpp"
#include "render_system/fog/DataRoles.hpp"
#include "render_system/fog/ShadowDispatchResourceProvider.hpp"
#include "render_system/fog/util/CreateBuffers.hpp"
#include "renderer/volume/ComputePipelineLayout.hpp"
#include "renderer/volume/ContainerRenderResourceData.hpp"
#include "renderer/volume/VolumeFrameRoles.hpp"
#include "renderer/volume/VolumePipelineRecipe.hpp"
#include "wrappers/graphics/policies/SubmitDescriptorRequestsPolicy.hpp"

#include <star_terrain/rendering/DataRoles.hpp>
#include <star_terrain/rendering/TerrainShadowRenderPhase.hpp>

#include <star_common/FrameTracker.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/core/device/system/event/ManagerRequest.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>
#include <starlight/core/renderer/RenderPhase.hpp>
#include <starlight/event/DescriptorPoolReady.hpp>

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
        star::StarTextures::Texture::Builder(context.getDevice())
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
    render_system::fog::policies::ShadowResourceResolutionPolicy resPolicy,
    star::ManagerController::RenderResource::Buffer *instanceManagerInfo,
    star::ManagerController::RenderResource::Buffer *instanceNormalInfo,
    std::shared_ptr<star::core::renderer::FrameData> frameData, star::Handle offscreenPhaseHandle,
    std::string vdbFilePath, std::shared_ptr<star::StarCamera> camera, const std::array<glm::vec4, 2> &aabbBounds)
    : m_shadowResourceProvider(std::move(resPolicy)), m_infoManagerInstanceModel(instanceManagerInfo),
      m_infoManagerInstanceNormal(instanceNormalInfo), m_frameData(std::move(frameData)),
      m_offscreenPhaseHandle(std::move(offscreenPhaseHandle)), m_vdbFilePath(std::move(vdbFilePath)),
      m_camera(std::move(camera)), m_aabbBounds(aabbBounds)
{
}

static VolumeDirectoryProcessor GetVolumeProcessor(std::string vdbFilePath) noexcept
{
    // need to find a way to tell what type the volume is...
    // dragon is level set
    const auto tmpDir = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::tmp_directory));
    VolumeDirectoryProcessor processor(vdbFilePath, tmpDir);
    processor.init();

    return processor;
}

std::unique_ptr<star::core::renderer::RenderPhase> VolumeRenderPhaseProvider::build(
    star::core::device::DeviceContext &c, star::core::renderer::RenderPhaseRegistry &phases)
{
    const vk::Extent2D screensize = c.getEngineResolution();
    const uint8_t numFramesInFlight = c.frameTracker().getSetup().getNumFramesInFlight();
    const size_t n = static_cast<size_t>(numFramesInFlight);

    auto phase = std::make_unique<VolumeRenderPhase>();
    phase->m_device = c.getDevice().getVulkanDevice();
    phase->m_cmdBus = &c.getCmdBus();
    phase->m_frameData = m_frameData;
    phase->m_cameraRole = star::core::renderer::roleHandle(star::core::renderer::frame_roles::Camera);

    auto *offscreenPhase = phases.getPhase(m_offscreenPhaseHandle);
    assert(offscreenPhase != nullptr && "offscreen render phase must be built before the volume render phase");
    phase->m_offscreenPhase = offscreenPhase;

    {
        auto *shadowPhase = phases.getPhase(m_shadowTerrainPhaseHandle);
        assert(shadowPhase != nullptr && "shadow render phase must be built before the volume render phase");
        phase->m_terrainShadowPhase = shadowPhase;
    }

    phase->m_timelineSemaphores = CreateSemaphores(c.getEventBus(), c.frameTracker());

    auto submitter = std::make_shared<star::wrappers::graphics::policies::SubmitDescriptorRequestsPolicy>(
        VolumeRenderPhase::getDescriptorRequests(numFramesInFlight));
    submitter->init(c.getEventBus());

    phase->m_fogController = std::make_shared<FogInfoController>();
    phase->m_volumeFrameData = std::make_shared<star::core::renderer::FrameData>();
    phase->m_volumeFrameData->add(phase->m_fogController,
                                  star::core::renderer::roleHandle(renderer::volume::frame_roles::Fog));
    phase->m_volumeFrameData->add(star::core::renderer::FrameData::BorrowedBuffer{m_infoManagerInstanceModel},
                                  star::core::renderer::roleHandle(renderer::volume::frame_roles::InstanceModel));
    phase->m_volumeFrameData->add(star::core::renderer::FrameData::BorrowedBuffer{m_infoManagerInstanceNormal},
                                  star::core::renderer::roleHandle(renderer::volume::frame_roles::InstanceNormal));

    m_shadowResourceProvider.addResourcesTo(c, *phase->m_volumeFrameData);

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

    auto processor = GetVolumeProcessor(m_vdbFilePath);
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

    phase->m_activeRayStorage.resize(numFramesInFlight);
    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        phase->aabbInfoBuffers.emplace_back(star::ManagerRenderResource::addRequest(
            c.getDeviceID(), std::make_unique<AABBTransfer>(graphicsQueueFamilyIndex, m_aabbBounds)));

        const std::string cstr = std::to_string(i);
        phase->m_activeRayStorage[i] = std::make_shared<star::StarBuffers::Buffer>(
            render_system::fog::CreateActiveRayStorageBuffer(c, "RMEM_" + cstr, c.getEngineResolution()));
    }

    const auto randomTexRole = star::core::renderer::roleHandle("RandomTex");
    const auto vdbRole = star::core::renderer::roleHandle("Vdb");
    const auto cameraExtraRole = star::core::renderer::roleHandle("CameraExtra");
    const auto activeRayRole = star::core::renderer::roleHandle("ActiveRay");
    const auto depthRole = star::core::renderer::roleHandle("Depth");
    const auto colorRole = star::core::renderer::roleHandle("Color");
    const auto outputRole = star::core::renderer::roleHandle("Output");
    const auto shadowMapRole = star::core::renderer::roleHandle(data_roles::TerrainShadowMap);
    const auto shadowDepthRole = star::core::renderer::roleHandle(data_roles::TerrainShadowDepthRaw);
    const auto transmittanceMapShadowRole =
        star::core::renderer::roleHandle(render_system::fog::data_roles::LightTransmittanceMap);

    const auto staticInfo = star::core::renderer::shaderInfoHandle("Static");
    const auto dynamicInfo = star::core::renderer::shaderInfoHandle("Dynamic");
    const auto shadowInfo = star::core::renderer::shaderInfoHandle("Shadow");
    const auto depthSceneInfo = star::core::renderer::shaderInfoHandle("DepthScene");
    const auto depthShadowInfo = star::core::renderer::shaderInfoHandle("DepthShadow");

    phase->m_volumeFrameData->add(
        star::core::renderer::FrameData::FixedTextureHandle{.handle = phase->randomValueTexture,
                                                            .layout = vk::ImageLayout::eGeneral,
                                                            .format = vk::Format::eR32G32B32A32Sfloat},
        randomTexRole);
    phase->m_volumeFrameData->add(star::core::renderer::FrameData::FixedBufferHandle{.handle = phase->vdbInfoFog},
                                  vdbRole);
    phase->m_volumeFrameData->add(star::core::renderer::FrameData::FixedBufferHandle{.handle = phase->cameraShaderInfo},
                                  cameraExtraRole);
    phase->m_volumeFrameData->add(star::core::renderer::FrameData::OwnedBuffer{.buffers = phase->m_activeRayStorage},
                                  activeRayRole);
    phase->m_volumeFrameData->add(star::core::renderer::FrameData::OwnedTexture{.textures = phase->computeWriteToImages,
                                                                                .layout = vk::ImageLayout::eGeneral,
                                                                                .format = vk::Format::eR8G8B8A8Unorm},
                                  outputRole);

    m_shadowResourceProvider.addResourcesTo(c, *phase->m_volumeFrameData);

    auto builder = star::core::renderer::DescriptorRecipe::Builder(
        c.getEventBus(), c, star::event::DescriptorPoolReady::GetUniqueTypeName());
    builder.setShaderInfoOut(staticInfo, &phase->m_staticShaderInfo);
    {
        const star::Handle shadowRole =
            star::core::renderer::roleHandle(star::terrain::rendering::data_roles::ShadowLightProjections);

        assert(m_shadowTerrainPhaseHandle.isInitialized() && "Shadow registration was never provided");

        auto *shadowPhase = phases.getPhase(m_shadowTerrainPhaseHandle);
        assert(shadowPhase != nullptr && "Terrain shadow phase must be defined before volume");
        std::vector<const star::StarTextures::Texture *> shadowMaps(n);
        for (size_t i = 0; i < n; i++)
        {
            shadowMaps[i] = &c.getImageManager().get(shadowPhase->getRenderTargets().depthHandles()[i])->texture;
        }

        phase->m_volumeFrameData->add(
            star::core::renderer::FrameData::BorrowedTexture{.textures = std::move(shadowMaps),
                                                             .layout = vk::ImageLayout::eShaderReadOnlyOptimal},
            shadowMapRole);
        // shadow light projections: set 1 of the static shader info (current = staticInfo).
        builder.addBinding(shadowPhase->getFrameData(), 1, 6, shadowRole,
                           vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);

        // Non-compare (raw) sun depth for the transmittance pass's rayInit depth test.
        // Same shadow depth images as shadowMapRole, but with a non-compare sampler for
        // raw reads via texelFetch.
        std::vector<const star::StarTextures::Texture *> rawShadowDepths;
        rawShadowDepths.reserve(n);
        for (size_t i = 0; i < n; i++)
        {
            rawShadowDepths.push_back(
                &c.getImageManager()
                     .get(static_cast<star::terrain::TerrainShadowRenderPhase *>(shadowPhase)->rawDepthHandles()[i])
                     ->texture);
        }
        phase->m_volumeFrameData->add(
            star::core::renderer::FrameData::BorrowedTexture{.textures = std::move(rawShadowDepths),
                                                             .layout = vk::ImageLayout::eShaderReadOnlyOptimal},
            shadowDepthRole);
    }

    {
        std::vector<const star::StarTextures::Texture *> depthTextures;
        depthTextures.reserve(n);
        for (size_t i = 0; i < n; i++)
        {
            depthTextures.push_back(
                &c.getImageManager().get(offscreenPhase->getRenderTargets().depthHandles()[i])->texture);
        }
        phase->m_volumeFrameData->add(
            star::core::renderer::FrameData::BorrowedTexture{.textures = std::move(depthTextures),
                                                             .layout = vk::ImageLayout::eShaderReadOnlyOptimal},
            depthRole);
    }
    {
        std::vector<const star::StarTextures::Texture *> colorTextures;
        colorTextures.reserve(n);
        for (size_t i = 0; i < n; i++)
        {
            colorTextures.push_back(
                &c.getImageManager().get(offscreenPhase->getRenderTargets().colorHandles()[i])->texture);
        }
        phase->m_volumeFrameData->add(
            star::core::renderer::FrameData::BorrowedTexture{.textures = std::move(colorTextures),
                                                             .layout = vk::ImageLayout::eGeneral,
                                                             .format = vk::Format::eR8G8B8A8Unorm},
            colorRole);
    }

    phase->m_volumeFrameData->prepRender(c, numFramesInFlight);

    // current shader info = staticInfo (set above, before the shadow block).
    builder.addBinding(phase->m_volumeFrameData, 0, 0, randomTexRole, vk::DescriptorType::eStorageImage,
                       vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 0, 1, vdbRole, vk::DescriptorType::eStorageBuffer,
                    vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 0, 2, cameraExtraRole, vk::DescriptorType::eUniformBuffer,
                    vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 0, 3, activeRayRole, vk::DescriptorType::eStorageBuffer,
                    vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_frameData, 1, 0,
                    star::core::renderer::roleHandle(star::core::renderer::frame_roles::Camera),
                    vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_frameData, 1, 1,
                    star::core::renderer::roleHandle(star::core::renderer::frame_roles::LightInfo),
                    vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_frameData, 1, 2,
                    star::core::renderer::roleHandle(star::core::renderer::frame_roles::LightList),
                    vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 1, 3,
                    star::core::renderer::roleHandle(renderer::volume::frame_roles::InstanceModel),
                    vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 1, 4,
                    star::core::renderer::roleHandle(renderer::volume::frame_roles::InstanceNormal),
                    vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 1, 5,
                    star::core::renderer::roleHandle(renderer::volume::frame_roles::Fog),
                    vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
        .setShaderInfoOut(dynamicInfo, &phase->m_dynamicShaderInfo)
        // set 0 (dynamic): color / output -- depth moved to the depth set (set 3)
        .addBinding(phase->m_volumeFrameData, 0, 0, colorRole, vk::DescriptorType::eStorageImage,
                    vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 0, 1, outputRole, vk::DescriptorType::eStorageImage,
                    vk::ShaderStageFlagBits::eCompute)
        // transmittance map: also bound into the dynamic set at binding 2 (read by the
        // marched color/distance passes); the shadow set below owns bindings 0-2 for
        // the transmittance pass.
        .addBinding(phase->m_volumeFrameData, 0, 2, transmittanceMapShadowRole,
                    vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
        .setShaderInfoOut(shadowInfo, &phase->m_shadowShaderInfo)
        .addBinding(phase->m_volumeFrameData, 0, 0, colorRole,
                    vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 0, 1, outputRole,
                    vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 0, 2, transmittanceMapShadowRole,
                    vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
        .setShaderInfoOut(depthSceneInfo, &phase->m_sceneDepthShaderInfo)
        // depth-test image set (set 3): per-pass sampled inputs.
        // binding 0: depth (scene depth for color/distance, sun depth for transmittance).
        // binding 1: terrainShadowMapCompare (sampler2DShadow) -- same resource for all
        // passes; bound for the transmittance march as well (unused for now).
        .addBinding(phase->m_volumeFrameData, 0, 0, depthRole,
                    vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 0, 1, shadowMapRole,
                    vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute)
        .setShaderInfoOut(depthShadowInfo, &phase->m_shadowDepthShaderInfo)
        .addBinding(phase->m_volumeFrameData, 0, 0, shadowDepthRole,
                    vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute)
        .addBinding(phase->m_volumeFrameData, 0, 1, shadowMapRole,
                    vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute)
        .setOnShaderInfoReady(
            [layoutRecipe =
                 renderer::volume::ComputePipelineLayoutRecipe{
                     .context = &c,
                     .shaderInfos = {.staticInfo = &phase->m_staticShaderInfo,
                                     .dynamicInfo = &phase->m_dynamicShaderInfo,
                                     .depthInfo = &phase->m_sceneDepthShaderInfo},
                     .out = &phase->computePipelineLayout},
             recipes =
                 std::vector<renderer::volume::VolumePipelineRecipe>{
                     {.context = &c,
                      .shaderFile = "volume_debugColorRedActiveRays.comp",
                      .outHandle = &phase->nanoVDBPipeline_hitBoundingBox},
                     {.context = &c,
                      .shaderFile = "volume_nanoVDBSurface.comp",
                      .outHandle = &phase->nanoVDBPipeline_surface},
                     {.context = &c, .shaderFile = "volume_color.comp", .outHandle = &phase->marchedPipeline},
                     {.context = &c, .shaderFile = "volume_linear.comp", .outHandle = &phase->linearPipeline},
                     {.context = &c, .shaderFile = "volume_exp.comp", .outHandle = &phase->expPipeline},
                     {.context = &c,
                      .shaderFile = "volume_rayInit.comp",
                      .outHandle = &phase->m_initPipe,
                      .outCachedPipeline = &phase->m_pipeInfo.initPipeline},
                     {.context = &c,
                      .shaderFile = "volume_rayInit_lightCamera.comp",
                      .outHandle = &phase->m_initLightCameraPipe,
                      .outCachedPipeline = &phase->m_pipeInfo.initLightCameraPipeline},
                     {.context = &c,
                      .shaderFile = "volume_calcIndirectDispatch.comp",
                      .outHandle = &phase->m_indirectDispatchPipe,
                      .outCachedPipeline = &phase->m_pipeInfo.indirectDispatchPipeline},
                     {.context = &c,
                      .shaderFile = "volume_precomputeLightTransmittance.comp",
                      .outHandle = &phase->m_precomputeLightTransmittancePipe,
                      .outCachedPipeline = &phase->m_pipeInfo.transmittancePipe.pipeline}}](
                star::core::device::DeviceContext &) mutable {
                layoutRecipe();

                const vk::PipelineLayout &layout = *layoutRecipe.out->get();
                for (auto &recipe : recipes)
                {
                    recipe.layout = &layout;
                    recipe();
                }
            })
        .build();

    renderer::volume::ContainerRenderResourceData pipelineData{
        .inputs{.fogController = phase->m_fogController.get(),
                .aabbInfoBuffers = &phase->aabbInfoBuffers,
                .offscreenRenderToColors = &offscreenPhase->getRenderTargets().colorHandles(),
                .offscreenRenderToDepths = &offscreenPhase->getRenderTargets().depthHandles(),
                .instanceManagerInfo = phase->m_volumeFrameData->getController(
                    star::core::renderer::roleHandle(renderer::volume::frame_roles::InstanceModel)),
                .instanceNormalInfo = phase->m_volumeFrameData->getController(
                    star::core::renderer::roleHandle(renderer::volume::frame_roles::InstanceNormal)),
                .globalInfoBuffers = phase->m_frameData->getController(
                    star::core::renderer::roleHandle(star::core::renderer::frame_roles::Camera)),
                .globalLightInfo = phase->m_frameData->getController(
                    star::core::renderer::roleHandle(star::core::renderer::frame_roles::LightInfo)),
                .globalLightList = phase->m_frameData->getController(
                    star::core::renderer::roleHandle(star::core::renderer::frame_roles::LightList)),
                .cameraShaderInfo = &phase->cameraShaderInfo,
                .vdbInfoFog = &phase->vdbInfoFog,
                .randomValueTexture = &phase->randomValueTexture},
        .outputs{.computeWriteToImages = &phase->computeWriteToImages,
                 .computeRayDistBuffers = &phase->computeRayDistanceBuffers,
                 .computeRayAtCutoffBuffer = &phase->computeRayAtCutoffDistanceBuffers}};

    phase->m_distanceComputer.prepRender(c, pipelineData, &phase->m_staticShaderInfo);

    phase->m_commandBuffer = c.getManagerCommandBuffer().submit(
        star::core::device::manager::ManagerCommandBuffer::Request{
            .recordBufferCallback = std::bind(&VolumeRenderPhase::recordCommandBuffer, phase.get(),
                                              std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            .order = star::Command_Buffer_Order::before_render_pass,
            .orderIndex = star::Command_Buffer_Order_Index::third,
            .type = star::Queue_Type::Tcompute,
            .waitStage = vk::PipelineStageFlagBits::eAllCommands,
            .willBeSubmittedEachFrame = true,
            .recordOnce = false,
            .overrideBufferSubmissionCallback = phase->getSubmissionOverride()},
        numFramesInFlight);

    auto cmd = star::command_order::DeclarePass(phase->m_commandBuffer, computeQueueFamilyIndex);
    c.begin().set(cmd).submit();

    auto *shadowPhase = phases.getPhase(m_shadowTerrainPhaseHandle);

    for (size_t i = 0; i < n; i++)
    {
        auto &ch = offscreenPhase->getRenderTargets().colorHandles()[i];
        phase->m_renderingContext.recordDependentImage.manualInsert(ch, &c.getImageManager().get(ch)->texture);
        auto &dh = offscreenPhase->getRenderTargets().depthHandles()[i];
        phase->m_renderingContext.recordDependentImage.manualInsert(dh, &c.getImageManager().get(dh)->texture);

        if (shadowPhase != nullptr)
        {
            auto &sh = shadowPhase->getRenderTargets().depthHandles()[i];
            phase->m_renderingContext.recordDependentImage.manualInsert(sh, &c.getImageManager().get(sh)->texture);
        }
    }

    phase->m_chunkHandler.prepRender(
        c, phase->m_commandBuffer,
        render_system::fog::FogDispatcher::DispatchContextInfo{
            .engineResolution = c.getEngineResolution(),
            .tranmittanceTextureResolution = vk::Extent3D().setHeight(256).setWidth(256).setDepth(256)},
        phase->isReady);

    // apply the initial fog config the application threaded in before build()
    phase->setFogInfo(std::move(m_fogInfo));
    phase->setFogType(m_fogType);
    if (m_transferNeighborHandle.has_value())
    {
        phase->setTransferNeighborHandle(m_transferNeighborHandle.value());
    }

    return phase;
}
