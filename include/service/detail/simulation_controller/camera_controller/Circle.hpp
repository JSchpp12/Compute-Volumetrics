#pragma once

#include <starlight/virtual/StarCamera.hpp>

#include <glm/glm.hpp>

namespace service::simulation_controller::camera_controller
{
/// @brief Controller which rotates the camera around in a circle
class Circle
{
  public:
    Circle() = default;
    Circle(glm::vec3 startCameraDirection, int numCameraPositions, float rotationDegreesPerTick = 1.0f);

    void tick(star::StarCamera &camera);
    void setRotationDegreesPerTick(float degrees);
    bool isDone() const;
    void reset(star::StarCamera &camera);
    void setNumCameraPositions(int numPositions);
    void setStartCameraDirection(glm::vec3 startCamDirection)
    {
        m_startCameraDirection = std::move(startCamDirection);
    }
    const glm::vec3 getStartCameraDirection() const
    {
        return m_startCameraDirection;
    }
    int getNumCameraPositions() const
    {
        return m_numCameraPositions;
    }
    float getRotationDegreesPerTick() const
    {
        return m_rotationDegreesPerTick;
    }

  private:
    glm::vec3 m_startCameraDirection{};
    int m_numCameraPositions{0};
    float m_rotationDegreesPerTick{1.0f};
    int m_counter{0};
};
} // namespace service::simulation_controller::camera_controller