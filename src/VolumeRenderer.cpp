#include "VolumeRenderer.hpp"

#include "CameraInfo.hpp"
#include "ConfigFile.hpp"
#include "FogControlInfo.hpp"
#include "ManagerRenderResource.hpp"

VolumeRenderer::VolumeRenderer(const std::shared_ptr<star::StarCamera> camera,
                               const std::vector<star::Handle> &instanceModelInfo,
                               std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToColors,
                               std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToDepths,
                               const std::vector<star::Handle> &globalInfoBuffers,
                               const std::vector<star::Handle> &sceneLightInfoBuffers,
                               const star::Handle &volumeTexture, const std::array<glm::vec4, 2> &aabbBounds)
    : offscreenRenderToColors(offscreenRenderToColors), offscreenRenderToDepths(offscreenRenderToDepths),
      globalInfoBuffers(globalInfoBuffers), sceneLightInfoBuffers(sceneLightInfoBuffers), aabbBounds(aabbBounds),
      camera(camera), instanceModelInfo(instanceModelInfo), volumeTexture(volumeTexture)
{
    this->cameraShaderInfo =
        star::ManagerRenderResource::addRequest(std::make_unique<CameraInfoController>(camera), true);
}

void VolumeRenderer::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    {
        std::array<const vk::ImageMemoryBarrier2, 3> prepForComputeDispatch{
            vk::ImageMemoryBarrier2()
                .setImage(this->offscreenRenderToColors->at(frameInFlightIndex)->getVulkanImage())
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
                                         .setAspectMask(vk::ImageAspectFlagBits::eDepth)),
            vk::ImageMemoryBarrier2()
                .setImage(this->computeWriteToImages.at(frameInFlightIndex)->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setNewLayout(vk::ImageLayout::eGeneral)
                .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
                .setDstAccessMask(vk::AccessFlagBits2::eNone)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1))};

        const auto depInfo =
            vk::DependencyInfo().setImageMemoryBarrierCount(3).setPImageMemoryBarriers(&prepForComputeDispatch.front());

        commandBuffer.pipelineBarrier2(depInfo);
    }

    switch (this->currentFogType)
    {
    case (FogType::marched):
        this->marchedPipeline->bind(commandBuffer);
        break;
    case (FogType::linear):
        this->linearPipeline->bind(commandBuffer);
        break;
    case (FogType::exp):
        this->expPipeline->bind(commandBuffer);
        break;
    default:
        throw std::runtime_error("Unsupported type");
    }

    auto sets = this->compShaderInfo->getDescriptors(frameInFlightIndex);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *this->computePipelineLayout, 0,
                                     static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

    commandBuffer.dispatch(this->workgroupSize.x, this->workgroupSize.y, 1);

    // give render to image back to graphics queue

    std::array<vk::ImageMemoryBarrier2, 2> backToGraphics{
        vk::ImageMemoryBarrier2()
            .setImage(this->offscreenRenderToColors->at(frameInFlightIndex)->getVulkanImage())
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
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
            .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests)
            .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
            .setSrcQueueFamilyIndex(*this->computeQueueFamilyIndex)
            .setDstQueueFamilyIndex(*this->graphicsQueueFamilyIndex)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1))};

    auto deps = vk::DependencyInfo().setImageMemoryBarrierCount(2).setPImageMemoryBarriers(&backToGraphics.front());

    commandBuffer.pipelineBarrier2(deps);
}

star::Command_Buffer_Order_Index VolumeRenderer::getCommandBufferOrderIndex()
{
    return star::Command_Buffer_Order_Index::second;
}

star::Command_Buffer_Order VolumeRenderer::getCommandBufferOrder()
{
    return star::Command_Buffer_Order::before_render_pass;
}

star::Queue_Type VolumeRenderer::getCommandBufferType()
{
    return star::Queue_Type::Tcompute;
}

vk::PipelineStageFlags VolumeRenderer::getWaitStages()
{
    return vk::PipelineStageFlagBits::eComputeShader;
}

bool VolumeRenderer::getWillBeSubmittedEachFrame()
{
    return true;
}

bool VolumeRenderer::getWillBeRecordedOnce()
{
    return false;
}

void VolumeRenderer::initResources(star::StarDevice &device, const int &numFramesInFlight,
                                   const vk::Extent2D &screensize)
{
    this->workgroupSize = CalculateWorkGroupSize(screensize);

    {
        const uint32_t computeIndex = device.getQueueFamily(star::Queue_Type::Tcompute).getQueueFamilyIndex();

        this->graphicsQueueFamilyIndex =
            std::make_unique<uint32_t>(device.getQueueFamily(star::Queue_Type::Tgraphics).getQueueFamilyIndex());
        if (*this->graphicsQueueFamilyIndex != computeIndex)
        {
            this->computeQueueFamilyIndex = std::make_unique<uint32_t>(uint32_t(computeIndex));
        }
    }

    this->displaySize = std::make_unique<vk::Extent2D>(screensize);
    {
        uint32_t indices[] = {device.getQueueFamily(star::Queue_Type::Tcompute).getQueueFamilyIndex(),
                              device.getQueueFamily(star::Queue_Type::Tgraphics).getQueueFamilyIndex()};

        auto builder =
            star::StarTexture::Builder(device.getDevice(), device.getAllocator().get())
                .setCreateInfo(star::Allocator::AllocationBuilder()
                                   .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                   .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                                   .build(),
                               vk::ImageCreateInfo()
                                   .setExtent(vk::Extent3D()
                                                  .setWidth(static_cast<int>(this->displaySize->width))
                                                  .setHeight(static_cast<int>(this->displaySize->height))
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
                                   .setSharingMode(vk::SharingMode::eConcurrent),
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
                                    .setMaxAnisotropy(star::StarTexture::SelectAnisotropyLevel(
                                        device.getPhysicalDevice().getProperties()))
                                    .setMagFilter(star::StarTexture::SelectTextureFiltering(
                                        device.getPhysicalDevice().getProperties()))
                                    .setMinFilter(star::StarTexture::SelectTextureFiltering(
                                        device.getPhysicalDevice().getProperties()))
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
            this->computeWriteToImages.emplace_back(builder.build());

            // set the layout to general for compute shader use
            auto oneTime = device.beginSingleTimeCommands();

            vk::ImageMemoryBarrier barrier{};
            barrier.sType = vk::StructureType::eImageMemoryBarrier;
            barrier.oldLayout = vk::ImageLayout::eUndefined;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

            barrier.image = this->computeWriteToImages.back()->getVulkanImage();
            barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;

            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
            barrier.subresourceRange.levelCount = 1;   // image is not an array
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            oneTime->buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,     // which pipeline stages should
                                                                                         // occurr before barrier
                                              vk::PipelineStageFlagBits::eComputeShader, // pipeline stage in
                                                                                         // which operations will
                                                                                         // wait on the barrier
                                              {}, {}, nullptr, barrier);

            device.endSingleTimeCommands(std::move(oneTime));
        }
    }

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        this->aabbInfoBuffers.emplace_back(
            star::ManagerRenderResource::addRequest(std::make_unique<AABBController>(this->aabbBounds)));

        this->fogControlShaderInfo.emplace_back(star::ManagerRenderResource::addRequest(
            std::make_unique<FogControlInfoController>(i, this->fogControlInfo)));
    }
}

void VolumeRenderer::destroyResources(star::StarDevice &device)
{
    this->compShaderInfo.reset();

    for (auto &computeWriteToImage : this->computeWriteToImages)
    {
        computeWriteToImage.reset();
    }

    this->marchedPipeline.reset();
    this->linearPipeline.reset();
    this->expPipeline.reset();
    device.getDevice().destroyPipelineLayout(*this->computePipelineLayout);
}

std::vector<std::pair<vk::DescriptorType, const int>> VolumeRenderer::getDescriptorRequests(
    const int &numFramesInFlight)
{
    return std::vector<std::pair<vk::DescriptorType, const int>>{
        std::make_pair(vk::DescriptorType::eStorageImage, (3 * numFramesInFlight)),
        std::make_pair(vk::DescriptorType::eUniformBuffer, 1 + (4 * numFramesInFlight)),
        std::make_pair(vk::DescriptorType::eStorageBuffer, 1),
        std::make_pair(vk::DescriptorType::eCombinedImageSampler, 1)};
}

void VolumeRenderer::createDescriptors(star::StarDevice &device, const int &numFramesInFlight)
{
    auto shaderInfoBuilder =
        star::StarShaderInfo::Builder(device, numFramesInFlight)
            .addSetLayout(star::StarDescriptorSetLayout::Builder(device)
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build())
            .addSetLayout(star::StarDescriptorSetLayout::Builder(device)
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                              .build())
            .addSetLayout(star::StarDescriptorSetLayout::Builder(device)
                              .addBinding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                              .build())
            .addSetLayout(star::StarDescriptorSetLayout::Builder(device)
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build());

    for (int i = 0; i < numFramesInFlight; i++)
    {
        shaderInfoBuilder.startOnFrameIndex(i)
            .startSet()
            .add(this->globalInfoBuffers.at(i), false)
            .add(this->sceneLightInfoBuffers.at(i), false)
            .startSet()
            .add(this->cameraShaderInfo, false)
            .add(this->volumeTexture, vk::ImageLayout::eGeneral, true)
            .startSet()
            .add(*this->offscreenRenderToColors->at(i), vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm, false)
            .add(*this->offscreenRenderToDepths->at(i), vk::ImageLayout::eShaderReadOnlyOptimal, false)
            .add(*this->computeWriteToImages.at(i), vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm, false)
            .startSet()
            .add(this->instanceModelInfo.at(i), false)
            .add(this->aabbInfoBuffers.at(i), false)
            .add(this->fogControlShaderInfo.at(i), false);
    }

    this->compShaderInfo = shaderInfoBuilder.build();

    {
        auto sets = this->compShaderInfo->getDescriptorSetLayouts();
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
        pipelineLayoutInfo.pSetLayouts = sets.data();
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(sets.size());
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        this->computePipelineLayout =
            std::make_unique<vk::PipelineLayout>(device.getDevice().createPipelineLayout(pipelineLayoutInfo));
    }

    std::string compShaderPath =
        star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "shaders/volumeRenderer/volume.comp";
    auto compShader = star::StarShader(compShaderPath, star::Shader_Stage::compute);

    this->marchedPipeline =
        std::make_unique<star::StarComputePipeline>(device, *this->computePipelineLayout, compShader);
    this->marchedPipeline->init();

    std::string linearFogPath =
        star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "shaders/volumeRenderer/linearFog.comp";
    auto linearCompShader = star::StarShader(linearFogPath, star::Shader_Stage::compute);
    this->linearPipeline =
        std::make_unique<star::StarComputePipeline>(device, *this->computePipelineLayout, linearCompShader);
    this->linearPipeline->init();

    const std::string expFogPath =
        star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "shaders/volumeRenderer/expFog.comp";
    auto expCompShader = star::StarShader(expFogPath, star::Shader_Stage::compute);
    this->expPipeline =
        std::make_unique<star::StarComputePipeline>(device, *this->computePipelineLayout, expCompShader);
    this->expPipeline->init();
}

glm::uvec2 VolumeRenderer::CalculateWorkGroupSize(const vk::Extent2D &screenSize)
{
    const int threadsPerWorkgroup = 8;

    return glm::uvec2{std::ceil(screenSize.width / 8), std::ceil(screenSize.height / 8)};
}