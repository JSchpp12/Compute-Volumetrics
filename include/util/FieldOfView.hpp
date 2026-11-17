#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace util
{
/// @brief Azimuth of a ground-plane view direction, in radians, measured from +Z towards +X.
inline double GetViewAzimuthRadians(const glm::vec3 &viewDirection)
{
    return std::atan2(static_cast<double>(viewDirection.x), static_cast<double>(viewDirection.z));
}

/// @brief Total yaw rotation a circle controller applies across its camera positions, in degrees.
/// @details The controller places the camera at its start direction first, then rotates once for every remaining
/// position, so a controller with N positions sweeps (N - 1) ticks of rotation.
inline double GetRotationSpanDegrees(int numCameraPositions, float rotationDegreesPerTick)
{
    return static_cast<double>(std::max(numCameraPositions - 1, 0)) * static_cast<double>(rotationDegreesPerTick);
}

/// @brief Widens a rotation span by a camera's horizontal field of view, clamped to a full turn.
/// @details The camera sees half its field of view on either side of every direction it takes.
inline double GetViewSpanDegrees(double rotationSpanDegrees, float horizontalFieldOfViewDegrees)
{
    return std::min(rotationSpanDegrees + static_cast<double>(horizontalFieldOfViewDegrees), 360.0);
}

/// @brief Azimuth of the leading edge of a viewed span, in radians.
/// @details The camera already sees half its horizontal field of view before its view direction, so the leading edge
/// sits half a field of view ahead of the view azimuth.
inline double GetLeadingEdgeAzimuthRadians(double viewAzimuthRadians, float horizontalFieldOfViewDegrees)
{
    return viewAzimuthRadians - glm::radians(static_cast<double>(horizontalFieldOfViewDegrees) / 2.0);
}
} // namespace util
