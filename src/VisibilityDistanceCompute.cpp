#include "VisibilityDistanceCompute.hpp"

#include <starlight/common/ConfigFile.hpp>

void VisibilityDistanceCompute::prepRender(star::core::device::DeviceContext &context)
{
    buildPipelines(context.getGraphicsManagers());
}

void VisibilityDistanceCompute::recordCommandBuffer(vk::CommandBuffer commandBuffer,
                                                    const glm::uvec2 &workgroupSize,
                                                    const star::StarShaderInfo &volumeShaderInfo, Fog::Type type)
{

}

void VisibilityDistanceCompute::buildPipelines(star::core::device::manager::GraphicsContainer &graphicsManagers)
{
    assert(m_sharedPipelineLayout != nullptr);
    auto shaderPath = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) /
                      "shaders" / "volumeRenderer" / "volume_distance.comp";

    m_pipelineMarched =
        graphicsManagers.pipelineManager->submit(star::core::device::manager::PipelineRequest{star::StarPipeline(
            star::StarPipeline::ComputePipelineConfigSettings(), *m_sharedPipelineLayout,
            std::vector<star::Handle>{graphicsManagers.shaderManager->submit(star::core::device::manager::ShaderRequest{
                star::StarShader(shaderPath.string(), star::Shader_Stage::compute),
                star::Compiler("PNANOVDB_GLSL")})})});
}
