#pragma once

#include <nlohmann/json.hpp>

namespace service::simulation_controller::camera_controller
{
    class Circle; 
    void to_json(nlohmann::json &j, const Circle &c); 
    void from_json(const nlohmann::json &j, Circle &c); 
}