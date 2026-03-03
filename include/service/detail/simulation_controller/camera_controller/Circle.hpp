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
    Circle(glm::vec3 startCameraDirection, int numCameraPositions);

    void tick(star::StarCamera &camera);
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

  private:
    glm::vec3 m_startCameraDirection{};
    int m_numCameraPositions{0};
    int m_counter{0};
};
} // namespace service::simulation_controller::camera_controller