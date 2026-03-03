#pragma once

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

} // namespace Fog
