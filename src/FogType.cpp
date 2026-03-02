#include "FogType.hpp"

namespace Fog
{
std::string TypeToString(const Type &type)
{
    switch (type)
    {
    case (Type::sLinear):
        return "linear";
    case (Type::sExponential):
        return "exponential";
    case (Type::sMarched):
        return "marched";
    case (Type::sMarchedHomogenous):
        return "marched_homogenous";
    case (Type::sNanoBoundingBox):
        return "nano_boundingBox";
    case (Type::sNanoSurface):
        return "nano_surface";
    default:
        return "";
    }
}
} // namespace Fog