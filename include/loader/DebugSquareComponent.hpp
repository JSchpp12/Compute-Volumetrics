#pragma once

#include <vector>
#include <cstdint>

#include <starlight/debug/DebugPrimitives.hpp>

namespace loader
{
struct DebugCubeComponent
{
    std::vector<star::primitive::CubeDesc> cubeInfos;
    uint8_t numberOfDebugSquares; 
};
}