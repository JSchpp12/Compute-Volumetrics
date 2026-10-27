#include "render_system/fog/ShaderFactory.hpp"

#include <starlight/command/shader/CreateShader.hpp>
#include <starlight/core/CommandBus.hpp>
#include <starlight/core/ShaderMacroDefinitions.hpp>

namespace render_system::fog
{
static std::filesystem::path GetVolumeShaderDir()
{
    return std::filesystem::path{"shaders"} / "volumeRenderer";
}

static star::core::ShaderMacroDefinitions PnanoVdbDefinitions()
{
    star::core::ShaderMacroDefinitions macroDefinitions;
    macroDefinitions.addMacroDefinition({.name = "PNANOVDB_GLSL"});
    return macroDefinitions;
}

star::Handle CreateVolumeComputeShader(const std::string &shaderPath, star::core::CommandBus &commandBus)
{
    auto fullShaderPath = GetVolumeShaderDir() / shaderPath;
    star::command::shader::CreateShader cmd;
    cmd.setStage(star::Shader_Stage::compute)
        .setPath(std::move(fullShaderPath))
        .setMacroDefinitions(PnanoVdbDefinitions());

    commandBus.submit(cmd);
    return cmd.getReply().get();
}
} // namespace render_system::fog
