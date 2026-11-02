#include "renderer/volume/VolumePipelineRecipe.hpp"

#include <starlight/core/CommandBus.hpp>

#include <vulkan/vulkan.hpp>

namespace renderer::volume
{
star::Handle VolumePipelineRecipe::buildPipeline(star::core::device::DeviceContext *context) const
{
    assert(context != nullptr && "Device context should not be null");
    assert(layout != nullptr && "Pipeline layout should not be null");
    assert(shader.isInitialized() && "Pipeline recipe requires an initialized shader handle");

    star::command::pipeline::CreatePipeline cmd;
    cmd.setComputePipeline().addShaderHandle(shader).setPipelineLayout(*layout);

    if (pipelineConstantsProvider != nullptr)
    {
        pipelineConstantsProvider(cmd);
    }

    context->getCmdBus().submit(cmd);
    return cmd.getReply().get();
}

int VolumePipelineRecipe::operator()()
{
    *outHandle = buildPipeline(context);
    return 0;
}
} // namespace renderer::volume