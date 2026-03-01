#include "service/detail/simulation_controller/CameraController.hpp"

namespace service::simulation_controller
{
CameraController::CameraController(camera_controller::Circle controller) : m_controller(std::move(controller))
{
}

bool CameraController::operator!() const
{
    return m_controller.has_value();
}

void CameraController::tick(star::StarCamera &camera)
{
    assert(m_controller.has_value() && "No controller has been registered");

    if (m_controller)
    {
        if (auto *c = std::get_if<service::simulation_controller::camera_controller::Circle>(&m_controller.value()))
        {
            c->tick(camera);
        }
    }
}

void CameraController::reset(star::StarCamera &camera)
{
    if (m_controller)
    {
        if (auto *c = std::get_if<service::simulation_controller::camera_controller::Circle>(&m_controller.value()))
        {
            c->reset(camera);
        }
    }
}

bool CameraController::isDone() const
{
    if (m_controller)
    {
        if (const auto *c =
                std::get_if<service::simulation_controller::camera_controller::Circle>(&m_controller.value()))
        {
            return c->isDone();
        }
    }
}

} // namespace service::simulation_controller