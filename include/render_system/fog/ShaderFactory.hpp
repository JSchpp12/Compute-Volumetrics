#pragma once

#include <star_common/Handle.hpp>

#include <string>

namespace star::core
{
class CommandBus;
}

namespace render_system::fog
{
star::Handle CreateVolumeComputeShader(const std::string &shaderPath, star::core::CommandBus &commandBus);
} // namespace render_system::fog
