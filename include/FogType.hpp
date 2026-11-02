#pragma once

#include <string>

namespace Fog
{
enum Type
{
    sNone,
    sLinear,
    sExponential,
    sMarched,
    sCountOfNonDebugTypes,
    sNanoBoundingBox,
    sNanoSurface,
    sCount
};

std::string TypeToString(const Type &type);

Type TypeFromString(const std::string &str);

inline bool NeedsTransmittancePrecompute(const Type &type) noexcept
{
    return type == Type::sLinear || type == Type::sExponential ? false : true;
}

} // namespace Fog
