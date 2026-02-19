#include "FogType.hpp"

namespace Fog
{
std::string TypeToString(const Type &type)
{
    switch (type)
    {
    case (Type::linear):
        return "linear";
    case (Type::exp):
        return "exponential";
    case (Type::marched):
        return "marched";
    case (Type::marched_homogenous):
        return "marched_homogenous";
    case (Type::nano_boundingBox):
        return "nano_boundingBox";
    case (Type::nano_surface):
        return "nano_surface";
    default:
        return "";
    }
}
} // namespace Fog