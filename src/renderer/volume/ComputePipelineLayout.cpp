#include "renderer/volume/ComputePipelineLayout.hpp"

#include "renderer/PipelineLayoutAssembler.hpp"

#include "render_system/fog/struct/ShaderPushInfo.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace renderer::volume
{
int ComputePipelineLayoutRecipe::operator()()
{
    assert(context && out && "ComputePipelineLayoutRecipe requires context and an output target");

    PipelineLayoutAssembler assembler{
        .device = &context->getDevice(),
        .pushConstants = {vk::PushConstantRange()
                              .setSize(sizeof(render_system::fog::ShaderPushInfo))
                              .setOffset(0)
                              .setStageFlags(vk::ShaderStageFlagBits::eCompute)}};

    // Primary contributions: their set layouts are baked into the pipeline layout.
    // baseSet is read from each StarShaderInfo (set by the DescriptorRecipe).
    auto pushPrimary = [&](const ShaderInfoEntry &e) {
        assert(e.info && *e.info &&
               "ComputePipelineLayoutRecipe requires all primary shader-info entries to be provided");
        assembler.contributions.push_back({e.info->get(), /*primary=*/true});
    };
    pushPrimary(shaderInfos.staticInfo);
    pushPrimary(shaderInfos.dynamicInfo);
    pushPrimary(shaderInfos.depthInfo);

    // Validation-only contributions: bind against the same layout at the same set
    // indices but were built separately. Checked compatible, not baked in.
    for (const auto &v : validationInfos)
    {
        if (v.info && *v.info)
            assembler.contributions.push_back({v.info->get(), /*primary=*/false});
    }

    *out = std::make_unique<vk::PipelineLayout>(assembler());
    return 0;
}
} // namespace renderer::volume