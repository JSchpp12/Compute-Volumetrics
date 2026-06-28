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

Type TypeFromString(const std::string &str)
{
    if (str == "linear")
        return Type::sLinear;
    if (str == "exponential")
        return Type::sExponential;
    if (str == "marched")
        return Type::sMarched;
    if (str == "marched_homogenous")
        return Type::sMarchedHomogenous;
    if (str == "nano_boundingBox")
        return Type::sNanoBoundingBox;
    if (str == "nano_surface")
        return Type::sNanoSurface;
    return Type::sNone;
}
} // namespace Fog