#include "controller/CircleCameraController.hpp"

#include <starlight/command/FileIO/ReadFromFile.hpp>

#include "controller/detail/SimulationBoundsFile.hpp"

CircleCameraController::CircleCameraController(std::shared_ptr<star::StarCamera> camera)
    : m_rotationCounter(0), m_camera(std::move(m_camera))
{
}

void CircleCameraController::init(star::core::device::DeviceContext &context)
{
    controller::SimulationBoundsFile file{""};
    m_loadedBounds = file.getFuture(); 

    star::job::tasks::io::ReadPayload payload{"", std::move(file)};

    auto readTask = star::job::tasks::io::CreateReadTask(std::move(payload));
    auto readCmd = star::command::file_io::ReadFromFile(std::move(readTask));

    context.begin().set(readCmd).submit();
}

void CircleCameraController::frameUpdate(star::core::device::DeviceContext &context)
{
    //check if the bounds are loaded

}