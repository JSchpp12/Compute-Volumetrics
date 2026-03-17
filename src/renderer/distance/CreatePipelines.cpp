#include "renderer/distance/CreatePipelines.hpp"

namespace renderer::distance
{
static star::Handle DefaultPoolHandle()
{
    return {.type = star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                star::core::device::manager::GetDescriptorPoolTypeName),
            .id = 0};
}

static std::unique_ptr<star::StarShaderInfo> BuildShaderInfo(
    star::Handle &deviceID, star::core::device::StarDevice &device,
    star::core::device::manager::GraphicsContainer &graphicsManagers, uint8_t numFramesInFlight)
{
    auto shaderInfoBuilder =
        star::StarShaderInfo::Builder(deviceID, device,
                                      *graphicsManagers.descriptorPoolManager->get(DefaultPoolHandle())->pool,
                                      numFramesInFlight)
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(device))
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(3, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(device))
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
                              .build(device))
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(device)); 
}

static star::Handle BuildPipeline(const std::filesystem::path &shaderDir, const std::string &shaderFile,
                                  const vk::PipelineLayout computePipelineLayout,
                                  star::core::device::manager::GraphicsContainer &graphicsManagers)
{
}

static vk::PipelineLayout BuildPipelineLayout()
{
}

void CreatePipelines::create()
{
    //*m_volumeShaderInfo = BuildShaderInfo();
}

} // namespace renderer::distance