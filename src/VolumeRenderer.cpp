#include "VolumeRenderer.hpp"

#include "CameraInfo.hpp"
#include "ConfigFile.hpp"
#include "FogControlInfo.hpp"
#include "ManagerRenderResource.hpp"

VolumeRenderer::VolumeRenderer(star::StarCamera &camera, const std::vector<star::Handle> &instanceModelInfo,
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
    vk::ImageMemoryBarrier prepOffscreenImages{};
    prepOffscreenImages.sType = vk::StructureType::eImageMemoryBarrier;
    prepOffscreenImages.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    prepOffscreenImages.newLayout = vk::ImageLayout::eGeneral;
    prepOffscreenImages.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    prepOffscreenImages.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    prepOffscreenImages.image = this->offscreenRenderToColors->at(frameInFlightIndex)->getVulkanImage();
    prepOffscreenImages.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    prepOffscreenImages.subresourceRange.baseMipLevel = 0;
    prepOffscreenImages.subresourceRange.levelCount = 1;
    prepOffscreenImages.subresourceRange.baseArrayLayer = 0;
    prepOffscreenImages.subresourceRange.layerCount = 1;
    prepOffscreenImages.srcAccessMask = vk::AccessFlagBits::eNone;
    prepOffscreenImages.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader, {},
                                  {}, nullptr, prepOffscreenImages);

    vk::ImageMemoryBarrier prepOffscreenDepths{};
    prepOffscreenDepths.sType = vk::StructureType::eImageMemoryBarrier;
    prepOffscreenDepths.oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    prepOffscreenDepths.newLayout = vk::ImageLayout::eGeneral;
    prepOffscreenDepths.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    prepOffscreenDepths.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    prepOffscreenDepths.image = this->offscreenRenderToDepths->at(frameInFlightIndex)->getVulkanImage();
    prepOffscreenDepths.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    prepOffscreenDepths.subresourceRange.baseMipLevel = 0;
    prepOffscreenDepths.subresourceRange.levelCount = 1;
    prepOffscreenDepths.subresourceRange.baseArrayLayer = 0;
    prepOffscreenDepths.subresourceRange.layerCount = 1;
    prepOffscreenDepths.srcAccessMask = vk::AccessFlagBits::eNone;
    prepOffscreenDepths.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader, {},
                                  {}, nullptr, prepOffscreenDepths);

    // transition image layout
    vk::ImageMemoryBarrier prepWriteToImages{};
    prepWriteToImages.sType = vk::StructureType::eImageMemoryBarrier;
    prepWriteToImages.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    prepWriteToImages.newLayout = vk::ImageLayout::eGeneral;
    prepWriteToImages.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    prepWriteToImages.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    prepWriteToImages.image = this->computeWriteToImages.at(frameInFlightIndex)->getVulkanImage();
    prepWriteToImages.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    prepWriteToImages.subresourceRange.baseMipLevel = 0;
    prepWriteToImages.subresourceRange.levelCount = 1;
    prepWriteToImages.subresourceRange.baseArrayLayer = 0;
    prepWriteToImages.subresourceRange.layerCount = 1;
    prepWriteToImages.srcAccessMask = vk::AccessFlagBits::eNone;
    prepWriteToImages.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader, {},
                                  {}, nullptr, prepWriteToImages);

    switch (this->currentFogType)
    {
    case (FogType::marched):
        this->marchedPipeline->bind(commandBuffer);
        break;
    case (FogType::linear):
        this->linearPipeline->bind(commandBuffer);
        break;
    default:
        throw std::runtime_error("Unsupported type");
    }

    auto sets = this->compShaderInfo->getDescriptors(frameInFlightIndex);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *this->computePipelineLayout, 0,
                                     static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

    commandBuffer.dispatch(80, 45, 1);
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

            oneTime.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,     // which pipeline stages should
                                                                               // occurr before barrier
                                    vk::PipelineStageFlagBits::eComputeShader, // pipeline stage in
                                                                               // which operations will
                                                                               // wait on the barrier
                                    {}, {}, nullptr, barrier);

            device.endSingleTimeCommands(oneTime);
        }
    }

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        this->aabbInfoBuffers.emplace_back(
            star::ManagerRenderResource::addRequest(std::make_unique<AABBController>(this->aabbBounds)));

        this->fogControlShaderInfo.emplace_back(star::ManagerRenderResource::addRequest(
            std::make_unique<FogControlInfoController>(i, this->fogNearDist, this->fogFarDist)));
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
    star::StarShaderInfo::Builder shaderInfoBuilder = star::StarShaderInfo::Builder(device, numFramesInFlight);
    shaderInfoBuilder
        .addSetLayout(star::StarDescriptorSetLayout::Builder(device)
                          .addBinding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                          .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute)
                          .addBinding(2, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                          .addBinding(3, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                          .addBinding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                          .addBinding(5, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                          .build())
        .addSetLayout(star::StarDescriptorSetLayout::Builder(device)
                          .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                          .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                          .build())
        .addSetLayout(star::StarDescriptorSetLayout::Builder(device)
                          .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                          .build());

    for (int i = 0; i < numFramesInFlight; i++)
    {
        // instance model info isnt setup
        shaderInfoBuilder.startOnFrameIndex(i)
            .startSet()
            .add(*this->offscreenRenderToColors->at(i), vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm, false)
            .add(*this->offscreenRenderToDepths->at(i), vk::ImageLayout::eShaderReadOnlyOptimal, false)
            .add(*this->computeWriteToImages.at(i), vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm, false)
            .add(this->cameraShaderInfo, false)
            .add(this->aabbInfoBuffers.at(i), false)
            .add(this->volumeTexture, vk::ImageLayout::eGeneral, true)
            .startSet()
            .add(this->globalInfoBuffers.at(i), false)
            .add(this->instanceModelInfo.at(i), false)
            .startSet()
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
}
