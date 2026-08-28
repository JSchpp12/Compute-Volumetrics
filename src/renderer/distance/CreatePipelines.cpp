#include "renderer/distance/CreatePipelines.hpp"

#include "renderer/PipelineLayoutAssembler.hpp"

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

#include "render_system/fog/struct/ShaderPushInfo.hpp"

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
    const auto shaderPath = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) / "shaders" / "volumeRenderer" / "volume_distance.comp";
    auto shaderRequest = graphicsManagers.shaderManager->submit(star::core::device::manager::ShaderRequest{
        star::StarShader(shaderPath.string(), star::Shader_Stage::compute), star::Compiler("PNANOVDB_GLSL")});

    return graphicsManagers.pipelineManager->submit(star::core::device::manager::PipelineRequest{
        star::PipelineProvider{std::move(shaderRequest), computePipelineLayout}});
}

void CreatePipelines::create()
{
    assert(outputs.dynamicShaderInfo != nullptr);
    *outputs.dynamicShaderInfo = buildShaderInfo();

    assert(outputs.pipelineLayout != nullptr);
    assert(inputs.staticComputeShaderInfo != nullptr);
    assert(inputs.staticComputeShaderInfo->get() && "shared static compute shader info must be built before distance pipelines");

    // The distance pipeline layout reuses the volume's shared static sets (0,1)
    // from staticComputeShaderInfo and appends the distance pass's own dynamic
    // set right after them. The dynamic set's baseSet is therefore the shared
    // static baseSet + the shared static set count -- derived, not hard-coded.
    star::StarShaderInfo *staticInfo = inputs.staticComputeShaderInfo->get();
    const uint32_t dynamicBaseSet = staticInfo->getBaseSet() + static_cast<uint32_t>(staticInfo->getDescriptorSetLayouts().size());
    outputs.dynamicShaderInfo->get()->setBaseSet(dynamicBaseSet);

    // Assembly goes through the shared PipelineLayoutAssembler; baseSet for each
    // contribution is read from the StarShaderInfo, so contiguity + compatibility
    // checks are the same as the volume path.
    renderer::PipelineLayoutAssembler assembler{
        .device = context.device,
        .pushConstants = {vk::PushConstantRange()
                              .setSize(sizeof(render_system::fog::ShaderPushInfo))
                              .setOffset(0)
                              .setStageFlags(vk::ShaderStageFlagBits::eCompute)}};
    assembler.contributions.push_back({inputs.staticComputeShaderInfo->get(), /*primary=*/true});
    assembler.contributions.push_back({outputs.dynamicShaderInfo->get(), /*primary=*/true});
    *outputs.pipelineLayout = assembler();

    assert(context.graphicsManagers != nullptr);
    *outputs.marchedPipeline = BuildPipeline(*outputs.pipelineLayout, *context.graphicsManagers);
}

} // namespace renderer::distance