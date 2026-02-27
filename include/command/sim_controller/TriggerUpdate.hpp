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
} // namespace trigger_update
struct TriggerUpdate : public star::common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return trigger_update::GetTypeName();
    }

    TriggerUpdate(Volume &volume, star::StarCamera &camera)
        : star::common::IServiceCommand(), volume(volume), camera(camera)
    {
    }

    TriggerUpdate(Volume &volume, star::StarCamera &camera, uint16_t type)
        : star::common::IServiceCommand(std::move(type)), volume(volume), camera(camera)
    {
    }

    Volume &volume;
    star::StarCamera &camera;
};

} // namespace sim_controller