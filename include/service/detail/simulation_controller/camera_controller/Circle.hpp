#pragma once

#include <starlight/virtual/StarCamera.hpp>

#include <glm/glm.hpp>

namespace simulation_controller::camera_controller
{
/// @brief Controller which rotates the camera around in a circle
class Circle
{
    public:
    Circle(int numCameraPositions, glm::vec3 startCameraDirection); 

    void frameUpdate(star::StarCamera &camera); 

    bool isDone() const; 

    void reset(star::StarCamera &camera); 

    private:
    glm::vec3 m_startCameraDirection; 
    int m_numCameraPositions; 
};
} // namespace simulation_controller::frame_controller