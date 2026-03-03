#include "service/detail/simulation_controller/FogEnabler.hpp"

#include "service/detail/simulation_controller/FogEnabler_json.hpp"

#include "FogType.hpp"

#include <cassert>

namespace service::simulation_controller
{

static void to_json(nlohmann::json &j, const std::array<bool, Fog::Type::sCountOfNonDebugTypes> status)
{
    assert(status.size() == 3);

    j["linear"] = status[static_cast<size_t>(Fog::Type::sLinear)];
    j["exponential"] = status[static_cast<size_t>(Fog::Type::sExponential)];
    j["marched_homogenous"] = status[static_cast<size_t>(Fog::Type::sMarchedHomogenous)];
    j["marched"] = status[static_cast<size_t>(Fog::Type::sMarched)];
}

static void from_json(const nlohmann::json &j, std::array<bool, Fog::Type::sCountOfNonDebugTypes> &status)
{
    status[static_cast<size_t>(Fog::Type::sLinear)] = j["linear"].get<bool>();
    status[static_cast<size_t>(Fog::Type::sExponential)] = j["exponential"].get<bool>();
    status[static_cast<size_t>(Fog::Type::sMarchedHomogenous)] = j["marched_homogenous"].get<bool>();
    status[static_cast<size_t>(Fog::Type::sMarched)] = j["marched"].get<bool>();
}

void to_json(nlohmann::json &j, const FogEnabler &data)
{
    nlohmann::json eJ;
    to_json(eJ, data.getEnabledStatus());
    j["enabled_fog_types"] = eJ;
}

void from_json(const nlohmann::json &j, FogEnabler &data)
{
    std::array<bool, static_cast<size_t>(Fog::Type::sCountOfNonDebugTypes)> status;
    from_json(j, status);
    data = FogEnabler(status);
}
} // namespace service::simulation_controller