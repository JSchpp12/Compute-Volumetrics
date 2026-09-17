#pragma once

#include <array>
#include <vector>

#include <starlight/debug/DebugPrimitives.hpp>

namespace loader
{
struct TransmittanceVizComponent
{
    std::vector<star::primitive::CubeDesc> cubeInfos;
    std::array<int, 3> windowSize{};
};
} // namespace loader
