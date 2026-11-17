#pragma once

#include <glm/glm.hpp>

#include <optional>

/// @brief Where the active simulation starts the camera and how far it sweeps it.
struct CameraSweepInfo
{
    /// @brief Direction the camera looks in when a sweep starts.
    std::optional<glm::vec3> startViewDirection{};
    /// @brief World-space position the simulation places the camera at, when the controller configures one.
    /// @details When unset, the simulation keeps the camera's current XY position and only lifts it above the ground.
    std::optional<glm::vec3> startCameraPosition{};
    /// @brief Number of distinct camera positions the controller steps through.
    int numCameraPositions{};
    /// @brief Yaw rotation applied between two consecutive camera positions, in degrees.
    float rotationDegreesPerTick{};
    /// @brief Total yaw the camera rotates through over a full sweep, in degrees.
    double rotationSpanDegrees{};
};
