#pragma once

#include <star_common/IServiceCommandWithReply.hpp>

#include <cstdint>
#include <string_view>
#include <utility>

namespace sim_controller
{
namespace get_simulation_focus_area_coverage
{
inline constexpr const char *GetTypeName()
{
    return "scGFAC";
}
} // namespace get_simulation_focus_area_coverage

/// @brief Coverage of the area the active simulation focuses on.
/// @details Placeholder payload; the fields depend on the coverage computation, which does not exist yet.
struct SimulationFocusAreaCoverage
{
};

/// @brief Queries the focus area coverage of the active simulation.
struct GetSimulationFocusAreaCoverage : public star::common::IServiceCommandWithReply<SimulationFocusAreaCoverage>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return get_simulation_focus_area_coverage::GetTypeName();
    }

    GetSimulationFocusAreaCoverage() = default;
    explicit GetSimulationFocusAreaCoverage(uint16_t type)
        : star::common::IServiceCommandWithReply<SimulationFocusAreaCoverage>(std::move(type))
    {
    }
};
} // namespace sim_controller
