#pragma once

#include "service/detail/simulation_controller/SimulationSteps.hpp"
#include "service/detail/simulation_controller/CameraController.hpp"
#include "service/detail/simulation_controller/FogEnabler.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <vector>

namespace service::simulation_controller
{
struct SimulationData
{
    SimulationSteps steps;
    CameraController cameraController;
    FogEnabler fogStatus;
    int initialCameraHeightAboveGround; 
    std::optional<glm::vec3> startCameraPosition;
};
} // namespace service::simulation_controller