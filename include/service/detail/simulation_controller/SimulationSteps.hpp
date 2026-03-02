#pragma once

#include "FogInfo.hpp"

namespace service::simulation_controller
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
} // namespace service::simulation_controller