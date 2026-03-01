#pragma once

#include "service/detail/simulation_controller/SimulationSteps.hpp"
#include "service/detail/simulation_controller/CameraController.hpp"

namespace service::simulation_controller
{
struct SimulationData
{
    SimulationSteps steps;
    CameraController cameraController;
};
} // namespace service::simulation_controller