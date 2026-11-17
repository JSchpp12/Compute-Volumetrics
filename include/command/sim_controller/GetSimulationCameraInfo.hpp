#pragma once

#include "structs/CameraSweepInfo.hpp"

#include <star_common/IServiceCommandWithReply.hpp>

#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

namespace sim_controller
{
namespace get_simulation_camera_info
{
inline constexpr const char *GetTypeName()
{
    return "scGCI";
}
} // namespace get_simulation_camera_info

/// @brief Queries the camera start position and rotation span of the active simulation.
struct GetSimulationCameraInfo : public star::common::IServiceCommandWithReply<std::optional<CameraSweepInfo>>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return get_simulation_camera_info::GetTypeName();
    }

    GetSimulationCameraInfo() = default;
    explicit GetSimulationCameraInfo(uint16_t type)
        : star::common::IServiceCommandWithReply<std::optional<CameraSweepInfo>>(std::move(type))
    {
    }
};
} // namespace sim_controller
