#pragma once

#include <string>

namespace Fog
{
enum Type
{
    sNone,
    sLinear,
    sExponential,
    sMarchedHomogenous,
    sMarched,
    sCountOfNonDebugTypes,
    sNanoBoundingBox,
    sNanoSurface,
    sCount
};

std::string TypeToString(const Type &type);

Type TypeFromString(const std::string &str);

} // namespace Fog
