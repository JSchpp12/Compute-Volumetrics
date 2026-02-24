#include "VolumeRendererCreateDescriptorsPolicy.hpp"

#include "ConfigFile.hpp"

#include <vulkan/vulkan.hpp>

void VolumeRendererCreateDescriptorsPolicy::create()
{
    createDescriptors();
}

std::unique_ptr<star::StarShaderInfo> VolumeRendererCreateDescriptorsPolicy::buildShaderInfo(bool useSDF)
{
    auto defaultPool = star::Handle{.type = star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                        star::core::device::manager::GetDescriptorPoolTypeName),
                                    .id = 0};

    auto shaderInfoBuilder =
        star::StarShaderInfo::Builder(*m_deviceID, *m_device,
                                      *m_graphicsManagers->descriptorPoolManager->get(defaultPool)->pool,
                                      m_numFramesInFlight)
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(*m_device))
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                              .build(*m_device))
            .addSetLayout(
                star::StarDescriptorSetLayout::Builder()
                    .addBinding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                    .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute)
                    .addBinding(2, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                    .addBinding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                    .addBinding(4, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                    .build(*m_device))
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(3, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(*m_device));

    for (uint8_t i = 0; i < m_numFramesInFlight; i++)
    {
        shaderInfoBuilder.startOnFrameIndex(i)
            .startSet()
            .add(m_infoManagerGlobalCamera->getHandle(i))
            .add(m_infoManagerSceneLightList->getHandle(i))
            .add(m_infoManagerSceneLightInfo->getHandle(i))
            .startSet()
            .add(*m_cameraShaderInfo);
        if (useSDF)
        {
            shaderInfoBuilder.add(*m_vdbInfoSDF);
        }
        else
        {
            shaderInfoBuilder.add(*m_vdbInfoFog);
        }
        shaderInfoBuilder.add(*m_randomValueTexture, vk::ImageLayout::eGeneral, vk::Format::eR32Sfloat);

        auto *colorTex = &m_graphicsManagers->imageManager.get(m_offscreenRenderToColors->at(i))->texture;
        auto *depthTex = &m_graphicsManagers->imageManager.get(m_offscreenRenderToDepths->at(i))->texture;
        shaderInfoBuilder.startSet()
            .add(*colorTex, vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm)
            .add(*depthTex, vk::ImageLayout::eShaderReadOnlyOptimal)
            .add(*m_computeWriteToImages->at(i), vk::ImageLayout::eGeneral, vk::Format::eR8G8B8A8Unorm)
            .add(m_computeRayDistBuffers->at(i))
            .add(m_computeRayAtCutoffBuffer->at(i))
            .startSet()
            .add(m_infoManagerInstanceModel->getHandle(i))
            .add(m_infoManagerInstanceNormal->getHandle(i))
            .add(m_aabbInfoBuffers->at(i))
            .add(m_fogController->getHandle(i),
                 &m_resourceManager->get<star::StarBuffers::Buffer>(*m_deviceID, m_fogController->getHandle(i))
                      ->resourceSemaphore);
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

void VolumeRendererCreateDescriptorsPolicy::createDescriptors()
{
    {
        *m_SDFShaderInfo = buildShaderInfo(true);
        *m_VolumeShaderInfo = buildShaderInfo(false);
    }

    {
        auto sets = m_SDFShaderInfo->get()->getDescriptorSetLayouts();
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
        pipelineLayoutInfo.pSetLayouts = sets.data();
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(sets.size());
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        *m_computePipelineLayout =
            std::make_unique<vk::PipelineLayout>(m_device->getVulkanDevice().createPipelineLayout(pipelineLayoutInfo));
    }

    const vk::PipelineLayout &cLay = *this->m_computePipelineLayout->get();

    auto shaderDir = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) /
                     "shaders" / "volumeRenderer";
    *m_nanoVDBPipeline_hitBoundingBox =
        BuildPipeline(shaderDir, "nanoVDBHitBoundingBox.comp", cLay, m_graphicsManagers);
    *m_nanoVDBPipeline_surface = BuildPipeline(shaderDir, "nanoVDBSurface.comp", cLay, m_graphicsManagers);
    *m_marchedPipeline = BuildPipeline(shaderDir, "volume.comp", cLay, m_graphicsManagers);
    *m_linearPipeline = BuildPipeline(shaderDir, "linearFog.comp", cLay, m_graphicsManagers);
    *m_expPipeline = BuildPipeline(shaderDir, "expFog.comp", cLay, m_graphicsManagers);
    *m_marchedHomogenousPipeline = BuildPipeline(shaderDir, "HomogenousMarchedFog.comp", cLay, m_graphicsManagers);
}