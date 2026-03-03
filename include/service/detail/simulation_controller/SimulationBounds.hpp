#pragma once

#include "FogInfo.hpp"

namespace service::simulation_controller
{
struct SimulationBounds
{
    FogInfo start;
    FogInfo stop;
    int numSteps;
};
} // namespace service::simulation_controller