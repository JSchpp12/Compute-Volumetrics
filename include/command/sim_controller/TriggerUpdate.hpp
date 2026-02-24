#pragma once

#include "Volume.hpp"

#include <starlight/virtual/StarCamera.hpp>

#include <star_common/IServiceCommand.hpp>

namespace sim_controller
{
namespace trigger_update
{
inline constexpr const char *GetTypeName()
{
    return "scTU";
}
}
struct TriggerUpdate : public star::common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return trigger_update::GetTypeName();
    }


    Volume &volume;
    star::StarCamera &camera;
};

} // namespace sim_controller