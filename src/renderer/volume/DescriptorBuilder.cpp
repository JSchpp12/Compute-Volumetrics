#include "renderer/volume/DescriptorBuilder.hpp"

#include "ConfigFile.hpp"
#include "render_system/fog/struct/ShaderPushInfo.hpp"

#include <vulkan/vulkan.hpp>

void DescriptorBuilder::create()
{
    createDescriptors();
}

static star::Handle DefaultPoolHandle()
{
    return {.type = star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                star::core::device::manager::GetDescriptorPoolTypeName),
            .id = 0};
}

std::unique_ptr<star::StarShaderInfo> DescriptorBuilder::buildStaticShaderInfo()
{
    auto shaderBuilder =
        star::StarShaderInfo::Builder(*m_deviceID, *m_device,
                                      *m_graphicsManagers->descriptorPoolManager->get(DefaultPoolHandle())->pool,
                                      m_numFramesInFlight)
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(4, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(*m_device))
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(3, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(5, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(*m_device));

    assert(m_data.inputs.activeRayStorageBuffers != nullptr &&
           m_data.inputs.activeRayStorageBuffers->size() == m_numFramesInFlight);
    assert(m_data.inputs.activeRayCountBuffers != nullptr &&
           m_data.inputs.activeRayStorageBuffers->size() == m_numFramesInFlight);

    for (uint8_t i{0}; i < m_numFramesInFlight; i++)
    {
        shaderBuilder.startOnFrameIndex(i)
            .startSet()
            .add(star::StarShaderInfo::TextureInfo{*m_data.inputs.randomValueTexture, vk::ImageLayout::eGeneral,
                                                   vk::Format::eR32G32B32A32Sfloat})
            .add(star::StarShaderInfo::BufferInfo{*m_data.inputs.vdbInfoFog})
            .add(star::StarShaderInfo::BufferInfo{*m_data.inputs.cameraShaderInfo})
            .add(star::StarShaderInfo::BufferInfo{&m_data.inputs.activeRayStorageBuffers->at(i)})
            .add(star::StarShaderInfo::BufferInfo{&m_data.inputs.activeRayCountBuffers->at(i)})
            .startSet()
            .add(star::StarShaderInfo::BufferInfo{m_data.inputs.globalInfoBuffers->getHandle(i)})
            .add(star::StarShaderInfo::BufferInfo{m_data.inputs.globalLightList->getHandle(i)})
            .add(star::StarShaderInfo::BufferInfo{m_data.inputs.globalLightInfo->getHandle(i)})
            .add(star::StarShaderInfo::BufferInfo{m_data.inputs.instanceManagerInfo->getHandle(i)})
            .add(star::StarShaderInfo::BufferInfo{m_data.inputs.instanceNormalInfo->getHandle(i)})
            .add(star::StarShaderInfo::BufferInfo{m_data.inputs.fogController->getHandle(i)},
                 &m_resourceManager
                      ->get<star::StarBuffers::Buffer>(*m_deviceID, m_data.inputs.fogController->getHandle(i))
                      ->resourceSemaphore);
    }

    return shaderBuilder.build();
}

std::unique_ptr<star::StarShaderInfo> DescriptorBuilder::buildShaderInfo()
{

    auto shaderInfoBuilder =
        star::StarShaderInfo::Builder(*m_deviceID, *m_device,
                                      *m_graphicsManagers->descriptorPoolManager->get(DefaultPoolHandle())->pool,
                                      m_numFramesInFlight)
            .addSetLayout(
                star::StarDescriptorSetLayout::Builder()
                    .addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute)
                    .addBinding(1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                    .addBinding(2, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                    .build(*m_device));

    for (uint8_t i = 0; i < m_numFramesInFlight; i++)
    {
        auto *colorTex = &m_graphicsManagers->imageManager.get(m_data.inputs.offscreenRenderToColors->at(i))->texture;
        auto *depthTex = &m_graphicsManagers->imageManager.get(m_data.inputs.offscreenRenderToDepths->at(i))->texture;

        shaderInfoBuilder.startOnFrameIndex(i)
            .startSet()
            .add(star::StarShaderInfo::TextureInfo{depthTex, vk::ImageLayout::eShaderReadOnlyOptimal})
            .add(star::StarShaderInfo::TextureInfo{colorTex, vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm})
            .add(star::StarShaderInfo::TextureInfo{m_data.outputs.computeWriteToImages->at(i).get(),
                                                   vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm});
    }

    return shaderInfoBuilder.build();
}

static star::Handle BuildPipeline(const std::filesystem::path &shaderDir, const std::string &shaderFile,
                                  const vk::PipelineLayout &compmutePipelineLayout,
                                  star::core::device::manager::GraphicsContainer *graphicsManagers)
{
    const auto fPath = shaderDir / shaderFile;
    return graphicsManagers->pipelineManager->submit(star::core::device::manager::PipelineRequest{star::StarPipeline(
        star::StarPipeline::ComputePipelineConfigSettings(), compmutePipelineLayout,
        std::vector<star::Handle>{graphicsManagers->shaderManager->submit(star::core::device::manager::ShaderRequest{
            star::StarShader(fPath.string(), star::Shader_Stage::compute), star::Compiler("PNANOVDB_GLSL")})})});
}

void DescriptorBuilder::createDescriptors()
{
    *m_staticShaderInfo = buildStaticShaderInfo();
    *m_dynamicShaderInfo = buildShaderInfo();

    {
        auto sets = m_staticShaderInfo->get()->getDescriptorSetLayouts();
        {
            auto dynamicSets = m_dynamicShaderInfo->get()->getDescriptorSetLayouts();
            sets.insert(sets.end(), dynamicSets.begin(), dynamicSets.end());
        }

        const auto pushRange = vk::PushConstantRange()
                                   .setSize(sizeof(render_system::fog::ShaderPushInfo))
                                   .setOffset(0)
                                   .setStageFlags(vk::ShaderStageFlagBits::eCompute);

        const auto layout = vk::PipelineLayoutCreateInfo()
                                .setPushConstantRangeCount(1)
                                .setPPushConstantRanges(&pushRange)
                                .setPSetLayouts(sets.data())
                                .setSetLayoutCount(static_cast<uint32_t>(sets.size()));

        *m_computePipelineLayout =
            std::make_unique<vk::PipelineLayout>(m_device->getVulkanDevice().createPipelineLayout(layout));
    }

    const vk::PipelineLayout &cLay = *this->m_computePipelineLayout->get();

    auto shaderDir = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) /
                     "shaders" / "volumeRenderer";
    *m_nanoVDBPipeline_hitBoundingBox =
        BuildPipeline(shaderDir, "nanoVDBHitBoundingBox.comp", cLay, m_graphicsManagers);
    *m_nanoVDBPipeline_surface = BuildPipeline(shaderDir, "nanoVDBSurface.comp", cLay, m_graphicsManagers);
    *m_marchedPipeline = BuildPipeline(shaderDir, "volume_color.comp", cLay, m_graphicsManagers);
    *m_linearPipeline = BuildPipeline(shaderDir, "linearFog.comp", cLay, m_graphicsManagers);
    *m_expPipeline = BuildPipeline(shaderDir, "expFog.comp", cLay, m_graphicsManagers);
    *m_marchedHomogenousPipeline = BuildPipeline(shaderDir, "HomogenousMarchedFog.comp", cLay, m_graphicsManagers);
    *m_initPipeline = BuildPipeline(shaderDir, "rayInit_BoundingBox_Depth.comp", cLay, m_graphicsManagers);
}