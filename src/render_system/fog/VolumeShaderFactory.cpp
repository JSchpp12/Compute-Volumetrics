#include "render_system/fog/VolumeShaderFactory.hpp"

#include <starlight/command/shader/CreateShader.hpp>
#include <starlight/core/ShaderMacroDefinitions.hpp>

namespace render_system::fog
{
static star::core::ShaderMacroDefinitions PnanoVdbDefinitions()
{
    star::core::ShaderMacroDefinitions macroDefinitions;
    macroDefinitions.addMacroDefinition({.name = "PNANOVDB_GLSL"});
    return macroDefinitions;
}

star::Handle CreateVolumeComputeShader(const std::filesystem::path &shaderPath, star::core::CommandBus &commandBus)
{
    star::command::shader::CreateShader cmd;
    cmd.setStage(star::Shader_Stage::compute).setPath(shaderPath).setMacroDefinitions(PnanoVdbDefinitions());

    commandBus.submit(cmd);
    return cmd.getReply().get();
}
} // namespace render_system::fog
