#pragma once

#include "FogInfo.hpp"

namespace controller::simulation_bounds_file
{
struct SimulationSteps
{
    bool operator!()
    {
        return numSteps == 0;
    }
    FogInfo start;
    FogInfo fogInfoChanges;
    int numSteps = 0;
};
} // namespace controller::simulation_bounds_file