#include "renderer/volume/VolumePipelineRecipe.hpp"

#include "ConfigFile.hpp"

#include <Compiler.hpp>
#include <Enums.hpp>
#include <StarPipeline.hpp>
#include <StarShader.hpp>
#include <device/managers/Pipeline.hpp>
#include <device/managers/Shader.hpp>
#include <starlight/command/pipeline/CreatePipeline.hpp>
#include <starlight/core/waiter/one_shot/WaiterFactory.hpp>

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <string>

namespace renderer::volume
{
static star::Handle BuildPipeline(const std::filesystem::path &shaderDir, const std::string &shaderFile,
                                  const vk::PipelineLayout &computePipelineLayout,
                                  star::core::device::DeviceContext *context)
{
    const auto fPath = shaderDir / shaderFile;

    star::command::pipeline::CreatePipeline cmd;
    cmd.setComputePipeline()
        .addShaderRequest({star::Shader_Stage::compute, fPath, star::Compiler("PNANOVDB_GLSL")})
        .setPipelineLayout(computePipelineLayout);

    context->getCmdBus().submit(cmd);
    return cmd.getReply().get();
}

static std::filesystem::path VolumeShaderDir()
{
    return std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) / "shaders" /
           "volumeRenderer";
}

int VolumePipelineRecipe::operator()()
{
    assert(layout != nullptr && "Shared pipeline layout should have been set before the () operator was called");

    *outHandle = BuildPipeline(VolumeShaderDir(), shaderFile, *layout, context);

    if (outCachedPipeline != nullptr)
    {
        star::core::waiter::one_shot::on_build_pipeline::BuildSetCachedPipeline(
            context->getEventBus(), context->getPipelineManager(), *outHandle, outCachedPipeline);
    }

    return 0;
}
} // namespace renderer::volume