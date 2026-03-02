#pragma once

namespace Fog
{
enum Type
{
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
