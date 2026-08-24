#include "renderer/volume/VolumePipelineRecipe.hpp"

#include "ConfigFile.hpp"

#include <Compiler.hpp>
#include <Enums.hpp>
#include <StarPipeline.hpp>
#include <StarShader.hpp>
#include <device/managers/Pipeline.hpp>
#include <device/managers/Shader.hpp>
#include <starlight/core/waiter/one_shot/WaiterFactory.hpp>

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <string>

namespace renderer::volume
{
static star::Handle BuildPipeline(const std::filesystem::path &shaderDir, const std::string &shaderFile,
                                  const vk::PipelineLayout &computePipelineLayout,
                                  star::core::device::manager::GraphicsContainer *graphicsManagers)
{
    const auto fPath = shaderDir / shaderFile;
    auto handle =
        graphicsManagers->pipelineManager->submit(star::core::device::manager::PipelineRequest{star::PipelineProvider(
            graphicsManagers->shaderManager->submit(star::core::device::manager::ShaderRequest{
                star::StarShader(fPath.string(), star::Shader_Stage::compute), star::Compiler("PNANOVDB_GLSL")}),
            computePipelineLayout)});

    return handle;
}

static std::filesystem::path VolumeShaderDir()
{
    return std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) / "shaders" /
           "volumeRenderer";
}

int VolumePipelineRecipe::operator()()
{
    assert(layout != nullptr && "Shared pipeline layout should have been set before the () operator was called");

    auto &graphicsManagers = context->getGraphicsManagers();
    *outHandle = BuildPipeline(VolumeShaderDir(), shaderFile, *layout, &graphicsManagers);

    if (outCachedPipeline != nullptr)
    {
        star::core::waiter::one_shot::on_build_pipeline::BuildSetCachedPipeline(
            context->getEventBus(), context->getPipelineManager(), *outHandle, outCachedPipeline);
    }

    return 0;
}
} // namespace renderer::volume