#pragma once

#include "Volume.hpp"

#include <starlight/virtual/StarCamera.hpp>

#include <star_common/IServiceCommandWithReply.hpp>

namespace sim_controller
{
namespace trigger_update
{
inline constexpr const char *GetTypeName()
{
    return "scTU";
}
} // namespace trigger_update

/// Contains what parameters controlled by the SimulationController were updated as result of the TriggerUpdate call
struct UpdateStatus
{
    bool cameraViewDirection{false};
    bool cameraLocation{false};
    bool fogParameters{false};
};

struct TriggerUpdate : public star::common::IServiceCommandWithReply<UpdateStatus>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return trigger_update::GetTypeName();
    }

    TriggerUpdate(Volume &volume, star::StarCamera &camera)
        : star::common::IServiceCommandWithReply<UpdateStatus>(), volume(volume), camera(camera)
    {
    }

    TriggerUpdate(Volume &volume, star::StarCamera &camera, uint16_t type)
        : star::common::IServiceCommandWithReply<UpdateStatus>(std::move(type)), volume(volume), camera(camera)
    {
    }

    Volume &volume;
    star::StarCamera &camera;
};

} // namespace sim_controller