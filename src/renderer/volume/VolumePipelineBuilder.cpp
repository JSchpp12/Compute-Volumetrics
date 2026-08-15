#include "renderer/volume/VolumePipelineBuilder.hpp"

#include "ConfigFile.hpp"
#include "render_system/fog/struct/ShaderPushInfo.hpp"

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
#include <vector>

namespace
{
static star::Handle BuildPipeline(const std::filesystem::path &shaderDir, const std::string &shaderFile,
                                  const vk::PipelineLayout &computePipelineLayout,
                                  star::core::device::manager::GraphicsContainer *graphicsManagers)
{
    const auto fPath = shaderDir / shaderFile;
    auto handle = graphicsManagers->pipelineManager->submit(star::core::device::manager::PipelineRequest{
        star::PipelineProvider(
            graphicsManagers->shaderManager->submit(star::core::device::manager::ShaderRequest{
                star::StarShader(fPath.string(), star::Shader_Stage::compute),
                star::Compiler("PNANOVDB_GLSL")}),
            computePipelineLayout)});

    return handle;
}
} // namespace

VolumePipelineBuilder::VolumePipelineBuilder(
    star::core::device::DeviceContext *context, std::unique_ptr<star::StarShaderInfo> *staticShaderInfo,
    std::unique_ptr<star::StarShaderInfo> *dynamicShaderInfo,
    std::unique_ptr<vk::PipelineLayout> *computePipelineLayout, star::Handle *marchedHomogenousPipeline,
    star::Handle *nanoVDBPipeline_hitBoundingBox, star::Handle *nanoVDBPipeline_surface,
    star::Handle *marchedPipeline, star::Handle *linearPipeline, star::Handle *expPipeline, star::Handle *initPipeline,
    vk::Pipeline *cachedInitPipeline, star::Handle *dispatchCmdPipeline, vk::Pipeline *cachedDispatchCmdPipeline)
    : m_context(context), m_staticShaderInfo(staticShaderInfo), m_dynamicShaderInfo(dynamicShaderInfo),
      m_computePipelineLayout(computePipelineLayout), m_marchedHomogenousPipeline(marchedHomogenousPipeline),
      m_nanoVDBPipeline_hitBoundingBox(nanoVDBPipeline_hitBoundingBox),
      m_nanoVDBPipeline_surface(nanoVDBPipeline_surface), m_marchedPipeline(marchedPipeline),
      m_linearPipeline(linearPipeline), m_expPipeline(expPipeline), m_initPipeline(initPipeline),
      m_cachedInitPipeline(cachedInitPipeline), m_dispatchCmdPipeline(dispatchCmdPipeline),
      m_cachedDispatchPipeline(cachedDispatchCmdPipeline)
{
}

void VolumePipelineBuilder::create()
{
    auto &c = *m_context;
    auto &device = c.getDevice();
    auto &graphicsManagers = c.getGraphicsManagers();

    // Assemble the compute pipeline layout from the static (2 sets) + dynamic
    // (1 set) layouts the recipe just built. Order: static set 0, static set 1,
    // dynamic set 0 -- matches how recordCommands binds descriptor sets.
    auto staticLayouts = m_staticShaderInfo->get()->getDescriptorSetLayouts();
    auto dynamicLayouts = m_dynamicShaderInfo->get()->getDescriptorSetLayouts();
    std::vector<vk::DescriptorSetLayout> sets;
    sets.insert(sets.end(), staticLayouts.begin(), staticLayouts.end());
    sets.insert(sets.end(), dynamicLayouts.begin(), dynamicLayouts.end());

    const auto pushRange = vk::PushConstantRange()
                               .setSize(sizeof(render_system::fog::ShaderPushInfo))
                               .setOffset(0)
                               .setStageFlags(vk::ShaderStageFlagBits::eCompute);

    const auto layout = vk::PipelineLayoutCreateInfo()
                            .setPushConstantRangeCount(1)
                            .setPPushConstantRanges(&pushRange)
                            .setPSetLayouts(sets.data())
                            .setSetLayoutCount(static_cast<uint32_t>(sets.size()));

    *m_computePipelineLayout =
        std::make_unique<vk::PipelineLayout>(device.getVulkanDevice().createPipelineLayout(layout));

    const vk::PipelineLayout &cLay = *m_computePipelineLayout->get();

    auto shaderDir = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) /
                     "shaders" / "volumeRenderer";
    *m_nanoVDBPipeline_hitBoundingBox =
        BuildPipeline(shaderDir, "volume_debugColorRedActiveRays.comp", cLay, &graphicsManagers);
    *m_nanoVDBPipeline_surface = BuildPipeline(shaderDir, "volume_nanoVDBSurface.comp", cLay, &graphicsManagers);
    *m_marchedPipeline = BuildPipeline(shaderDir, "volume_color.comp", cLay, &graphicsManagers);
    *m_linearPipeline = BuildPipeline(shaderDir, "volume_linear.comp", cLay, &graphicsManagers);
    *m_expPipeline = BuildPipeline(shaderDir, "volume_exp.comp", cLay, &graphicsManagers);
    *m_marchedHomogenousPipeline = BuildPipeline(shaderDir, "volume_homogenousMarch.comp", cLay, &graphicsManagers);
    *m_initPipeline = BuildPipeline(shaderDir, "volume_rayInit.comp", cLay, &graphicsManagers);
    *m_dispatchCmdPipeline = BuildPipeline(shaderDir, "volume_calcIndirectDispatch.comp", cLay, &graphicsManagers);

    star::core::waiter::one_shot::on_build_pipeline::BuildSetCachedPipeline(
        c.getEventBus(), c.getPipelineManager(), *m_dispatchCmdPipeline, m_cachedDispatchPipeline);

    star::core::waiter::one_shot::on_build_pipeline::BuildSetCachedPipeline(
        c.getEventBus(), c.getPipelineManager(), *m_initPipeline, m_cachedInitPipeline);
}
