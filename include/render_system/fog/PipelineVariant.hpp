#pragma once

#include <star_common/Handle.hpp>
#include <starlight/wrappers/graphics/StarPipelineConstants.hpp>

#include <string_view>

namespace render_system::fog
{
/// @brief Description of one pipeline variant: the stable name used to select it, the shader it is built from, and
/// the specialization constants that distinguish it from its siblings.
struct PipelineVariant
{
    std::string_view name;
    star::Handle shader;
    star::StarPipelineConstants constants;
};
} // namespace render_system::fog
