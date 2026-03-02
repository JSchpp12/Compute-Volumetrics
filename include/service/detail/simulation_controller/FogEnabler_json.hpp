#pragma once

#include <nlohmann/json.hpp>

namespace service::simulation_controller
{
class FogEnabler; 
void to_json(nlohmann::json &j, const FogEnabler &data); 
void from_json(const nlohmann::json &j, FogEnabler &data); 
}