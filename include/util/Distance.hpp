#pragma once

namespace util
{
inline float mileToMeters(float miles)
{
    return miles * 1609.34f;
}

inline float metersToMiles(float meters)
{
    return meters / 1609.34f;
}
} // namespace util