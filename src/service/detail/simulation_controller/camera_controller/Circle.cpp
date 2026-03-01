#include "service/detail/simulation_controller/camera_controller/Circle.hpp"

namespace service::simulation_controller::camera_controller
{
Circle::Circle(glm::vec3 startCameraDirection, int numCameraPositions)
    : m_startCameraDirection(std::move(startCameraDirection)), m_numCameraPositions(numCameraPositions)
{
}

void Circle::tick(star::StarCamera &camera)
{
    camera.rotateRelative(star::Type::Axis::y, 1.0);
    m_counter++;
}

bool Circle::isDone() const
{
    return m_counter == m_numCameraPositions;
}

void Circle::reset(star::StarCamera &camera)
{
    camera.setForwardVector(m_startCameraDirection);
    m_counter = 0;
}
} // namespace service::simulation_controller::camera_controller