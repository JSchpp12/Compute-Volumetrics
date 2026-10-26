#pragma once

#include <star_common/Handle.hpp>
#include <starlight/core/CommandBus.hpp>

#include <filesystem>

/// Builders for the volume pipeline's compute shaders. Free functions on purpose -- there is no
/// state to hold. The image memory barrier builders live next door in BarrierFactory.hpp.
namespace render_system::fog
{
/// Compile a single volume compute shader and return its pending handle. The PNanoVDB shaders read
/// the VDB header in GLSL mode, so the definition is supplied to the compiler rather than baked
/// into each shader. Throws when the shader fails to create.
star::Handle CreateVolumeComputeShader(const std::filesystem::path &shaderPath,
                                       star::core::CommandBus &commandBus);
} // namespace render_system::fog
