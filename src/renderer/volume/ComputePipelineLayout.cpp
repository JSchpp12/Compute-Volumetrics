#include "renderer/volume/ComputePipelineLayout.hpp"

#include "render_system/fog/struct/ShaderPushInfo.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>

namespace renderer::volume
{
int ComputePipelineLayoutRecipe::operator()()
{
    auto &device = context->getDevice();

    // Assemble the compute pipeline layout from the static (2 sets) + dynamic
    // (1 set) + depth (1 set) layouts. Order: static set 0, static set 1,
    // dynamic set 0 (pipeline set 2), depth set 0 (pipeline set 3) -- matches
    // how recordCommands binds descriptor sets.
    auto staticLayouts = shaderInfos.staticInfo->get()->getDescriptorSetLayouts();
    auto dynamicLayouts = shaderInfos.dynamicInfo->get()->getDescriptorSetLayouts();
    auto depthLayouts = shaderInfos.depthInfo->get()->getDescriptorSetLayouts();

    std::vector<vk::DescriptorSetLayout> sets;
    sets.insert(sets.end(), staticLayouts.begin(), staticLayouts.end());
    sets.insert(sets.end(), dynamicLayouts.begin(), dynamicLayouts.end());
    sets.insert(sets.end(), depthLayouts.begin(), depthLayouts.end());

    const auto pushRange = vk::PushConstantRange()
                               .setSize(sizeof(render_system::fog::ShaderPushInfo))
                               .setOffset(0)
                               .setStageFlags(vk::ShaderStageFlagBits::eCompute);

    const auto layout = vk::PipelineLayoutCreateInfo()
                            .setPushConstantRangeCount(1)
                            .setPPushConstantRanges(&pushRange)
                            .setPSetLayouts(sets.data())
                            .setSetLayoutCount(static_cast<uint32_t>(sets.size()));

    *out = std::make_unique<vk::PipelineLayout>(device.getVulkanDevice().createPipelineLayout(layout));
    return 0;
}
} // namespace renderer::volume