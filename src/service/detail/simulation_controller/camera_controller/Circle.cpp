#include "service/detail/simulation_controller/camera_controller/Circle.hpp"

namespace service::simulation_controller::camera_controller
{
Circle::Circle(glm::vec3 startCameraDirection, int numCameraPositions, float rotationDegreesPerTick)
    : m_startCameraDirection(std::move(startCameraDirection)), m_numCameraPositions(numCameraPositions),
      m_rotationDegreesPerTick(rotationDegreesPerTick)
{
}

void Circle::tick(star::StarCamera &camera)
{
    camera.rotateRelative(star::Type::Axis::y, m_rotationDegreesPerTick);
    m_counter++;
}

bool Circle::isDone() const
{
    return m_counter == m_numCameraPositions;
}

void Circle::setNumCameraPositions(int numPositions)
{
    if (numPositions > 360 || numPositions < 1)
    {
        STAR_THROW("Invalid value provided to setNumCameraPositions(). Must be between 1 and 360");
    }

    m_numCameraPositions = numPositions;
}

void Circle::setRotationDegreesPerTick(float degrees)
{
    if (degrees <= 0.0f || degrees > 360.0f)
    {
        STAR_THROW("Invalid value provided to setRotationDegreesPerTick(). Must be between 0 and 360");
    }

    m_rotationDegreesPerTick = degrees;
}

void Circle::reset(star::StarCamera &camera)
{
    camera.setForwardVector(m_startCameraDirection);
    m_counter = 1;
}
} // namespace service::simulation_controller::camera_controller