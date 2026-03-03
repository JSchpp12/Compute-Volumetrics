#pragma once

#include "service/detail/simulation_controller/SimulationSteps.hpp"
#include "service/detail/simulation_controller/CameraController.hpp"
#include "service/detail/simulation_controller/FogEnabler.hpp"

#include <vector>

namespace service::simulation_controller
{
struct SimulationData
{
    SimulationSteps steps;
    CameraController cameraController;
    FogEnabler fogStatus;
    int initialCameraHeightAboveGround; 
};
} // namespace service::simulation_controller