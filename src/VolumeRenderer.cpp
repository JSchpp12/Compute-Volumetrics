#include "VolumeRenderer.hpp"

#include "AABBTransfer.hpp"
#include "CameraInfo.hpp"
#include "ConfigFile.hpp"
#include "FogControlInfo.hpp"
#include "ManagerRenderResource.hpp"
#include "RandomValueTexture.hpp"
#include "VDBTransfer.hpp"

#include "FogData.hpp"
#include "LevelSetData.hpp"

VolumeRenderer::VolumeRenderer(std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceManagerInfo,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceNormalInfo,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalInfoBuffers,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneLightInfoBuffers,
                               std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneLightList,
                               std::string vdbFilePath, std::shared_ptr<FogInfo> fogControlInfo,
                               const std::shared_ptr<star::StarCamera> camera,
                               std::vector<star::StarTextures::Texture> *offscreenRenderToColors,
                               std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepths,
                               const std::array<glm::vec4, 2> &aabbBounds)
    : m_infoManagerInstanceModel(instanceManagerInfo), m_infoManagerInstanceNormal(instanceNormalInfo),
      m_infoManagerGlobalCamera(globalInfoBuffers), m_infoManagerSceneLightInfo(sceneLightInfoBuffers),
      m_infoManagerSceneLightList(sceneLightList), m_vdbFilePath(std::move(vdbFilePath)),
      m_fogController(fogControlInfo), offscreenRenderToColors(offscreenRenderToColors),
      offscreenRenderToDepths(offscreenRenderToDepths), aabbBounds(aabbBounds), camera(camera),
      volumeTexture(volumeTexture)
{
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
    m_renderingContext = buildRenderingContext(context);

    if (isRenderReady(context))
    {
        updateDependentData(context, frameInFlightIndex);
        gatherDependentExternalDataOrderingInfo(context, frameInFlightIndex);
    }
}

void VolumeRenderer::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                         const uint64_t &frameIndex)
{
    std::vector<vk::ImageMemoryBarrier2> prepareImages = std::vector<vk::ImageMemoryBarrier2>{
        vk::ImageMemoryBarrier2()
            .setImage(this->offscreenRenderToColors->at(frameInFlightIndex).getVulkanImage())
            .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex != nullptr ? *this->graphicsQueueFamilyIndex
                                                                             : vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(this->computeQueueFamilyIndex != nullptr ? *this->computeQueueFamilyIndex
                                                                             : vk::QueueFamilyIgnored)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)),
        vk::ImageMemoryBarrier2()
            .setImage(this->offscreenRenderToDepths->at(frameInFlightIndex)->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(this->computeQueueFamilyIndex != nullptr ? *this->graphicsQueueFamilyIndex
                                                                             : vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(this->computeQueueFamilyIndex != nullptr ? *this->computeQueueFamilyIndex
                                                                             : vk::QueueFamilyIgnored)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)
                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth))};

    if (this->isFirstPass)
    {
        prepareImages.push_back(vk::ImageMemoryBarrier2()
                                    .setImage(this->computeWriteToImages.at(frameInFlightIndex)->getVulkanImage())
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
        prepareImages.push_back(vk::ImageMemoryBarrier2()
                                    .setImage(this->computeWriteToImages.at(frameInFlightIndex)->getVulkanImage())
                                    .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                                    .setNewLayout(vk::ImageLayout::eGeneral)
                                    .setSrcQueueFamilyIndex(*this->graphicsQueueFamilyIndex)
                                    .setDstQueueFamilyIndex(*this->computeQueueFamilyIndex)
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

    commandBuffer.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(prepareImages));

    if (isReady)
    {
        m_renderingContext.pipeline->bind(commandBuffer);

        std::vector<vk::DescriptorSet> sets;
        if (this->currentFogType == FogType::marched)
        {
            sets = this->VolumeShaderInfo->getDescriptors(frameInFlightIndex);
        }
        else
        {
            sets = this->SDFShaderInfo->getDescriptors(frameInFlightIndex);
        }
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *this->computePipelineLayout, 0,
                                         static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

        commandBuffer.dispatch(this->workgroupSize.x, this->workgroupSize.y, 1);
    }

    // give render to image back to graphics queue
    std::array<vk::ImageMemoryBarrier2, 3> backToGraphics{
        vk::ImageMemoryBarrier2()
            .setImage(this->offscreenRenderToColors->at(frameInFlightIndex).getVulkanImage())
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
            .setDstAccessMask(vk::AccessFlagBits2::eNone)
            .setSrcQueueFamilyIndex(*this->computeQueueFamilyIndex)
            .setDstQueueFamilyIndex(*this->graphicsQueueFamilyIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)),
        vk::ImageMemoryBarrier2()
            .setImage(this->offscreenRenderToDepths->at(frameInFlightIndex)->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                             vk::PipelineStageFlagBits2::eLateFragmentTests)
            .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead)
            .setSrcQueueFamilyIndex(*this->computeQueueFamilyIndex)
            .setDstQueueFamilyIndex(*this->graphicsQueueFamilyIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)),
        vk::ImageMemoryBarrier2()
            .setImage(this->computeWriteToImages.at(frameInFlightIndex)->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
            .setSrcQueueFamilyIndex(*this->computeQueueFamilyIndex)
            .setDstQueueFamilyIndex(*this->graphicsQueueFamilyIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1))};

    auto deps = vk::DependencyInfo().setImageMemoryBarrierCount(3).setPImageMemoryBarriers(&backToGraphics.front());

    commandBuffer.pipelineBarrier2(deps);
}

void VolumeRenderer::prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &screensize,
                                const uint8_t &numFramesInFlight)
{
    const auto camSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    this->cameraShaderInfo = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(camSemaphore)->semaphore,
        std::make_unique<CameraInfo>(
            camera, context.getDevice().getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex(),
            context.getDevice().getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment),
        nullptr, true);

    const auto vdbSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    this->vdbInfoSDF = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(vdbSemaphore)->semaphore,
        std::make_unique<VDBTransfer>(
            context.getDevice().getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex(),
            std::make_unique<LevelSetData>(m_vdbFilePath, openvdb::GridClass::GRID_LEVEL_SET)),
        nullptr, true);
    const auto vdbFogSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    this->vdbInfoFog = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(vdbFogSemaphore)->semaphore,
        std::make_unique<VDBTransfer>(
            context.getDevice().getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex(),
            std::make_unique<LevelSetData>(m_vdbFilePath, openvdb::GridClass::GRID_FOG_VOLUME)),
        nullptr, true);

    const auto randomSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    this->randomValueTexture = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(randomSemaphore)->semaphore,
        std::make_unique<RandomValueTexture>(
            screensize.width, screensize.height,
            context.getDevice().getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex(),
            context.getDevice().getPhysicalDevice().getProperties()));

    this->workgroupSize = CalculateWorkGroupSize(screensize);

    {
        const uint32_t computeIndex =
            context.getDevice().getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex();

        this->graphicsQueueFamilyIndex = std::make_unique<uint32_t>(
            context.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex());
        if (*this->graphicsQueueFamilyIndex != computeIndex)
        {
            this->computeQueueFamilyIndex = std::make_unique<uint32_t>(uint32_t(computeIndex));
        }
    }

    {
        uint32_t indices[] = {
            context.getDevice().getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex(),
            context.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex()};

        auto builder =
            star::StarTextures::Texture::Builder(context.getDevice().getVulkanDevice(),
                                                 context.getDevice().getAllocator().get())
                .setCreateInfo(star::Allocator::AllocationBuilder()
                                   .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                   .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                                   .build(),
                               vk::ImageCreateInfo()
                                   .setExtent(vk::Extent3D()
                                                  .setWidth(static_cast<int>(screensize.width))
                                                  .setHeight(static_cast<int>(screensize.height))
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

        for (uint8_t i = 0; i < numFramesInFlight; i++)
        {
            this->computeWriteToImages.emplace_back(builder.buildUnique());
        }
    }

    m_fogController.prepRender(context, numFramesInFlight);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        const auto aabbSemaphore =
            context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

        this->aabbInfoBuffers.emplace_back(star::ManagerRenderResource::addRequest(
            context.getDeviceID(), context.getSemaphoreManager().get(aabbSemaphore)->semaphore,
            std::make_unique<AABBTransfer>(
                context.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
                aabbBounds)));

        const auto fogShaderInfoSemaphore =
            context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));
    }

    commandBuffer = context.getManagerCommandBuffer().submit(
        star::core::device::manager::ManagerCommandBuffer::Request{
            .recordBufferCallback = std::bind(&VolumeRenderer::recordCommandBuffer, this, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3),
            .order = star::Command_Buffer_Order::before_render_pass,
            .orderIndex = star::Command_Buffer_Order_Index::second,
            .type = star::Queue_Type::Tcompute,
            .waitStage = vk::PipelineStageFlagBits::eComputeShader,
            .willBeSubmittedEachFrame = true,
            .recordOnce = false},
        context.getCurrentFrameIndex());
}

void VolumeRenderer::cleanupRender(star::core::device::DeviceContext &context)
{
    this->SDFShaderInfo->cleanupRender(context.getDevice());
    this->SDFShaderInfo.reset();

    this->VolumeShaderInfo->cleanupRender(context.getDevice());
    this->VolumeShaderInfo.reset();

    for (size_t i = 0; i < computeWriteToImages.size(); i++)
    {
        computeWriteToImages[i]->cleanupRender(context.getDevice().getVulkanDevice());
    }

    context.getDevice().getVulkanDevice().destroyPipelineLayout(*this->computePipelineLayout);
}

std::vector<std::pair<vk::DescriptorType, const int>> VolumeRenderer::getDescriptorRequests(
    const int &numFramesInFlight)
{
    return std::vector<std::pair<vk::DescriptorType, const int>>{
        std::make_pair(vk::DescriptorType::eStorageImage, 1 + (3 * numFramesInFlight)),
        std::make_pair(vk::DescriptorType::eUniformBuffer, 1 + (4 * numFramesInFlight)),
        std::make_pair(vk::DescriptorType::eStorageBuffer, 2),
        std::make_pair(vk::DescriptorType::eCombinedImageSampler, 1)};
}

void VolumeRenderer::createDescriptors(star::core::device::DeviceContext &device, const int &numFramesInFlight)
{
    this->SDFShaderInfo = buildShaderInfo(device, numFramesInFlight, true);
    this->VolumeShaderInfo = buildShaderInfo(device, numFramesInFlight, false);

    {
        auto sets = this->SDFShaderInfo->getDescriptorSetLayouts();
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
        pipelineLayoutInfo.pSetLayouts = sets.data();
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(sets.size());
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        this->computePipelineLayout = std::make_unique<vk::PipelineLayout>(
            device.getDevice().getVulkanDevice().createPipelineLayout(pipelineLayoutInfo));
    }

    {
        std::string compShaderPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) +
                                     "shaders/volumeRenderer/nanoVDBHitBoundingBox.comp";

        this->nanoVDBPipeline_hitBoundingBox =
            device.getPipelineManager().submit(star::core::device::manager::PipelineRequest{star::StarPipeline(
                star::StarPipeline::ComputePipelineConfigSettings(), *this->computePipelineLayout,
                std::vector<star::Handle>{device.getShaderManager().submit(star::core::device::manager::ShaderRequest{
                    star::StarShader(compShaderPath, star::Shader_Stage::compute),
                    star::Compiler("PNANOVDB_GLSL")})})});
    }

    {

        std::string compShaderPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) +
                                     "shaders/volumeRenderer/nanoVDBSurface.comp";

        this->nanoVDBPipeline_surface =
            device.getPipelineManager().submit(star::core::device::manager::PipelineRequest{star::StarPipeline(
                star::StarPipeline::ComputePipelineConfigSettings(), *this->computePipelineLayout,
                std::vector<star::Handle>{device.getShaderManager().submit(star::core::device::manager::ShaderRequest{
                    star::StarShader(compShaderPath, star::Shader_Stage::compute),
                    star::Compiler("PNANOVDB_GLSL")})})});
    }

    {
        std::string compShaderPath =
            star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "shaders/volumeRenderer/volume.comp";

        this->marchedPipeline =
            device.getPipelineManager().submit(star::core::device::manager::PipelineRequest{star::StarPipeline(
                star::StarPipeline::ComputePipelineConfigSettings(), *this->computePipelineLayout,
                std::vector<star::Handle>{device.getShaderManager().submit(star::core::device::manager::ShaderRequest{
                    star::StarShader(compShaderPath, star::Shader_Stage::compute),
                    star::Compiler("PNANOVDB_GLSL")})})});
    }

    std::string linearFogPath =
        star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "shaders/volumeRenderer/linearFog.comp";
    auto linearCompShader = star::StarShader(linearFogPath, star::Shader_Stage::compute);
    this->linearPipeline = device.getPipelineManager().submit(star::core::device::manager::PipelineRequest{
        star::StarPipeline(star::StarPipeline::ComputePipelineConfigSettings(), *this->computePipelineLayout,
                           std::vector<star::Handle>{device.getShaderManager().submit(
                               star::StarShader(linearFogPath, star::Shader_Stage::compute))})});

    const std::string expFogPath =
        star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "shaders/volumeRenderer/expFog.comp";
    auto expCompShader = star::StarShader(expFogPath, star::Shader_Stage::compute);

    this->expPipeline = device.getPipelineManager().submit(star::core::device::manager::PipelineRequest{
        star::StarPipeline(star::StarPipeline::ComputePipelineConfigSettings(), *this->computePipelineLayout,
                           std::vector<star::Handle>{device.getShaderManager().submit(
                               star::StarShader(expFogPath, star::Shader_Stage::compute))})});
}

std::unique_ptr<star::StarShaderInfo> VolumeRenderer::buildShaderInfo(star::core::device::DeviceContext &context,
                                                                      const uint8_t &numFramesInFlight,
                                                                      const bool &useSDF) const
{
    auto shaderInfoBuilder =
        star::StarShaderInfo::Builder(context.getDeviceID(), context.getDevice(), numFramesInFlight)
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(context.getDevice()))
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                              .build(context.getDevice()))
            .addSetLayout(
                star::StarDescriptorSetLayout::Builder()
                    .addBinding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                    .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute)
                    .addBinding(2, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                    .build(context.getDevice()))
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(3, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(context.getDevice()));

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        shaderInfoBuilder.startOnFrameIndex(i)
            .startSet()
            .add(m_infoManagerGlobalCamera->getHandle(i))
            .add(m_infoManagerSceneLightInfo->getHandle(i))
            .add(m_infoManagerSceneLightList->getHandle(i))
            .startSet()
            .add(this->cameraShaderInfo);
        if (useSDF)
        {
            shaderInfoBuilder.add(this->vdbInfoSDF);
        }
        else
        {
            shaderInfoBuilder.add(this->vdbInfoFog);
        }
        shaderInfoBuilder.add(this->randomValueTexture, vk::ImageLayout::eGeneral, vk::Format::eR32Sfloat);

        shaderInfoBuilder.startSet()
            .add(this->offscreenRenderToColors->at(i), vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm)
            .add(*this->offscreenRenderToDepths->at(i), vk::ImageLayout::eShaderReadOnlyOptimal)
            .add(*this->computeWriteToImages.at(i), vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm)
            .startSet()
            .add(m_infoManagerInstanceModel->getHandle(i))
            .add(this->aabbInfoBuffers.at(i))
            .add(m_fogController.getHandle(i),
                 &context.getManagerRenderResource()
                      .get<star::StarBuffers::Buffer>(context.getDeviceID(), m_fogController.getHandle(i))
                      ->resourceSemaphore)
            .add(m_infoManagerInstanceNormal->getHandle(i));
    }

    return shaderInfoBuilder.build();
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
            .m_manager.get(commandBuffer)
            .oneTimeWaitSemaphoreInfo.insert(m_fogController.getHandle(frameInFlightIndex), std::move(dataSemaphore),
                                             vk::PipelineStageFlagBits::eComputeShader);

        m_renderingContext.addBufferToRenderingContext(context, m_fogController.getHandle(frameInFlightIndex));
    }
}

star::core::renderer::RenderingContext VolumeRenderer::buildRenderingContext(star::core::device::DeviceContext &context)
{
    switch (this->currentFogType)
    {
    case (FogType::marched):
        return {.pipeline = &context.getPipelineManager().get(this->marchedPipeline)->request.pipeline};
        break;
    case (FogType::linear):
        return {.pipeline = &context.getPipelineManager().get(this->linearPipeline)->request.pipeline};
        break;
    case (FogType::exp):
        return {.pipeline = &context.getPipelineManager().get(this->expPipeline)->request.pipeline};
        break;
    case (FogType::nano_boundingBox):
        return {.pipeline = &context.getPipelineManager().get(this->nanoVDBPipeline_hitBoundingBox)->request.pipeline};
        break;
    case (FogType::nano_surface):
        return {.pipeline = &context.getPipelineManager().get(this->nanoVDBPipeline_surface)->request.pipeline};
        break;
    default:
        throw std::runtime_error("Unsupported type");
    }

    return star::core::renderer::RenderingContext{};
}

glm::uvec2 VolumeRenderer::CalculateWorkGroupSize(const vk::Extent2D &screenSize)
{
    return glm::uvec2{std::ceil(screenSize.width / 8), std::ceil(screenSize.height / 8)};
}