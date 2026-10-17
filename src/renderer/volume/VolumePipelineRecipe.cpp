#include "renderer/volume/VolumePipelineRecipe.hpp"

#include <Compiler.hpp>
#include <Enums.hpp>
#include <StarPipeline.hpp>
#include <StarShader.hpp>
#include <device/managers/Pipeline.hpp>
#include <device/managers/Shader.hpp>
#include <starlight/command/pipeline/CreatePipeline.hpp>
#include <starlight/core/CommandBus.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/waiter/one_shot/WaiterFactory.hpp>

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <sstream>
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
    const auto reply = cmd.getReply().get();
    if (!reply)
    {
        // The service reports resolution failures through the reply; a
        // pipeline that failed to build would otherwise stall the renderer
        // waiting for a pipeline-ready event that never fires.
        std::ostringstream oss;
        oss << "Failed to create volume pipeline for shader '" << fPath.string() << "':\n" << reply.error().toString();
        STAR_THROW(oss.str());
    }
    return reply.value();
}

static std::filesystem::path VolumeShaderDir()
{
    // Media-relative candidate: the PipelineCommandService resolves it through
    // the IOService (as-is -> media-relative -> exe-relative).
    return std::filesystem::path{"shaders"} / "volumeRenderer";
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