#include "renderer/distance/CreatePipelines.hpp"

#include <ConfigFile.hpp>

#include <Compiler.hpp>
#include <Enums.hpp>
#include <StarDescriptorBuilders.hpp>
#include <StarPipeline.hpp>
#include <StarShader.hpp>
#include <StarShaderInfo.hpp>
#include <cassert>
#include <cstdint>
#include <device/managers/DescriptorPool.hpp>
#include <device/managers/GraphicsContainer.hpp>
#include <device/managers/Pipeline.hpp>
#include <device/managers/Shader.hpp>
#include <filesystem>
#include <memory>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "render_system/FogShaderPushInfo.hpp"

namespace renderer::distance
{
static star::Handle DefaultPoolHandle()
{
    return {.type = star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                star::core::device::manager::GetDescriptorPoolTypeName),
            .id = 0};
}

std::unique_ptr<star::StarShaderInfo> CreatePipelines::buildShaderInfo() const
{
    assert(context.deviceID != nullptr);
    assert(context.device != nullptr);
    assert(context.graphicsManagers != nullptr);
    assert(context.resourceManger != nullptr);

    auto shaderInfoBuilder =
        star::StarShaderInfo::Builder(*context.deviceID, *context.device,
                                      *context.graphicsManagers->descriptorPoolManager->get(DefaultPoolHandle())->pool,
                                      numFramesInFlight)
            .addSetLayout(star::StarDescriptorSetLayout::Builder()
                              .addBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
                              .build(*context.device));

    for (uint8_t i{0}; i < numFramesInFlight; i++)
    {
        shaderInfoBuilder.startOnFrameIndex(i)
            .startSet()
            .add(star::StarShaderInfo::BufferInfo{&inputs.computeRayDistBuffers->at(i)})
            .add(star::StarShaderInfo::BufferInfo{&inputs.computeRayAtCutoffBuffers->at(i)});
    }

    return shaderInfoBuilder.build();
}

static star::Handle BuildPipeline(vk::PipelineLayout computePipelineLayout,
                                  star::core::device::manager::GraphicsContainer &graphicsManagers)
{
    const auto shaderPath = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) /
                            "shaders" / "volumeRenderer" / "volume_distance.comp";

    auto shaderRequest = graphicsManagers.shaderManager->submit(star::core::device::manager::ShaderRequest{
        star::StarShader(shaderPath.string(), star::Shader_Stage::compute), star::Compiler("PNANOVDB_GLSL")});

    return graphicsManagers.pipelineManager->submit(star::core::device::manager::PipelineRequest{star::StarPipeline{
        star::StarPipeline::ComputePipelineConfigSettings(), computePipelineLayout, {std::move(shaderRequest)}}});
}

static vk::PipelineLayout BuildPipelineLayout(std::array<vk::DescriptorSetLayout, 2> sharedStaticSet,
                                              vk::DescriptorSetLayout dynamicSet,
                                              star::core::device::StarDevice &device)
{
    const vk::DescriptorSetLayout sets[3]{sharedStaticSet[0], sharedStaticSet[1], dynamicSet};
    const auto range = vk::PushConstantRange()
                           .setSize(sizeof(render_system::FogShaderPushInfo))
                           .setOffset(0)
                           .setStageFlags(vk::ShaderStageFlagBits::eCompute);

    const auto info =
        vk::PipelineLayoutCreateInfo().setSetLayouts(sets).setPPushConstantRanges(&range).setPushConstantRangeCount(1);
    return device.getVulkanDevice().createPipelineLayout(info);
}

void CreatePipelines::create()
{
    assert(outputs.dynamicShaderInfo != nullptr);
    *outputs.dynamicShaderInfo = buildShaderInfo();

    assert(outputs.pipelineLayout != nullptr);
    *outputs.pipelineLayout =
        BuildPipelineLayout({inputs.staticComputeShaderInfo->get()->getDescriptorSetLayouts()[0],
                             inputs.staticComputeShaderInfo->get()->getDescriptorSetLayouts()[1]},
                            outputs.dynamicShaderInfo->get()->getDescriptorSetLayouts()[0], *context.device);

    assert(inputs.staticComputeShaderInfo != nullptr);
    assert(context.graphicsManagers != nullptr);
    *outputs.marchedPipeline = BuildPipeline(*outputs.pipelineLayout, *context.graphicsManagers);
}

} // namespace renderer::distance