#include "service/detail/simulation_controller/camera_controller/Circle.hpp"

#include "service/detail/simulation_controller/camera_controller/Circle_json.hpp"
#include "util/json/MathStructs.hpp"

namespace service::simulation_controller::camera_controller
{

void to_json(nlohmann::json &j, const Circle &c)
{
    j["num_camera_positions"] = c.getNumCameraPositions();
    {
        nlohmann::json dirData;
        util::to_json(dirData, c.getStartCameraDirection());
        j["start_camera_direction"] = dirData;
    }
}

void from_json(const nlohmann::json &j, Circle &c)
{
    glm::vec3 cameraStartDir;
    util::from_json(j["start_camera_direction"], cameraStartDir);
    int numPositions = j["num_camera_positions"].get<int>();

    c.setNumCameraPositions(numPositions); 
    c.setStartCameraDirection(std::move(cameraStartDir));
}
} // namespace service::simulation_controller::camera_controller