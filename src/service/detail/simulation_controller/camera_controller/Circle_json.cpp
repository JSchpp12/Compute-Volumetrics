#include "service/detail/simulation_controller/camera_controller/Circle.hpp"

#include <starlight/core/json/glm_json.hpp>
#include "service/detail/simulation_controller/camera_controller/Circle_json.hpp"

namespace service::simulation_controller::camera_controller
{

void to_json(nlohmann::json &j, const Circle &c)
{
    j["num_camera_positions"] = c.getNumCameraPositions();
    j["rotation_degree_per_tick"] = c.getRotationDegreesPerTick();
    j["start_camera_direction"] = c.getStartCameraDirection();
}

void from_json(const nlohmann::json &j, Circle &c)
{
    glm::vec3 cameraStartDir = j["start_camera_direction"];
    int numPositions = j.value("num_camera_positions", 360); 
    float rotationDegreesPerTick = j.value("rotation_degree_per_tick", 1.0f);

    c.setNumCameraPositions(numPositions);
    c.setRotationDegreesPerTick(rotationDegreesPerTick);
    c.setStartCameraDirection(std::move(cameraStartDir));
}
} // namespace service::simulation_controller::camera_controller