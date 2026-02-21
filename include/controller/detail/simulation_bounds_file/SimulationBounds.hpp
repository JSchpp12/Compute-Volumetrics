#pragma once

#include "FogInfo.hpp"

namespace controller::simulation_bounds_file
{
struct SimulationBounds
{
    FogInfo start;
    FogInfo stop;
    int numSteps;
};
} // namespace controller::simulation_bounds_file