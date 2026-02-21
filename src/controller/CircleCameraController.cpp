#include "controller/CircleCameraController.hpp"

#include "controller/detail/simulation_bounds_file/Reader.hpp"
#include "controller/detail/simulation_bounds_file/Writer.hpp"

#include <starlight/command/FileIO/ReadFromFile.hpp>
#include <starlight/command/FileIO/WriteToFile.hpp>

#include <star_common/helper/PathHelpers.hpp>

#include <filesystem>

CircleCameraController::CircleCameraController(std::shared_ptr<star::StarCamera> camera, std::shared_ptr<Volume> volume)
    : m_rotationCounter(0), m_camera(std::move(camera)), m_volume(std::move(volume))
{
}

void CircleCameraController::submitReadCmd(star::core::device::DeviceContext &context, const std::string &path)
{
    controller::simulation_bounds_file::Reader reader{};
    m_loadedInfo = reader.getFuture();
    star::job::tasks::io::ReadPayload payload{path, std::move(reader)};

    auto readTask = star::job::tasks::io::CreateReadTask(std::move(payload));
    auto readCmd = star::command::file_io::ReadFromFile(std::move(readTask));

    context.begin().set(readCmd).submit();
}

static void WriteDefaultControllerInfo(star::core::device::DeviceContext &context, const std::string &path)
{
    auto writePayload = star::job::tasks::io::WritePayload{path, controller::simulation_bounds_file::Writer{}};
    auto writeCmd = star::command::file_io::WriteToFile(star::job::tasks::io::CreateWriteTask(std::move(writePayload)));
    context.begin().set(writeCmd).submit();
}

void CircleCameraController::init(star::core::device::DeviceContext &context)
{
    const auto filePath = star::common::GetRuntimePath() / "SimulationController.json";

    if (std::filesystem::exists(filePath))
    {
        submitReadCmd(context, filePath.string());
    }
    else
    {
        std::promise<controller::simulation_bounds_file::SimulationSteps> promise;
        m_loadedInfo = promise.get_future();
        promise.set_value({});

        WriteDefaultControllerInfo(context, filePath.string());
    }

    m_rotationCounter = 361;
    m_stepCounter = 1000000;
}

void CircleCameraController::switchFogType(int newType)
{
    assert(m_volume && "Volume is not init");
    assert(m_camera && "Camera is not init");

    m_camera->setForwardVector(glm::vec3{0.0, 1.0, 0.0});
    m_volume->getRenderer().setFogInfo(m_loadedSteps.start);
    m_volume->getRenderer().setFogType(Fog::Type::linear);
}

void CircleCameraController::incrementLinear()
{
    assert(m_volume && "Volume is not init");

    m_volume->getRenderer().getFogInfo().linearInfo.farDist += m_loadedSteps.fogInfoChanges.linearInfo.farDist;
    m_volume->getRenderer().getFogInfo().linearInfo.nearDist += m_loadedSteps.fogInfoChanges.linearInfo.nearDist;
}

void CircleCameraController::frameUpdate(star::core::device::DeviceContext &context)
{
    assert(m_camera && "Camera is not initialized");

    // check if the bounds are loaded
    if (!m_loadedSteps)
    {
        m_loadedSteps = m_loadedInfo.get();
    }

    if (m_stepCounter >= m_loadedSteps.numSteps)
    {
        m_camera->rotateGlobal(star::Type::Axis::y, 1.0);
        switchFogType(1);

        m_stepCounter = 0;
    }

    switch (m_fogTypeTracker)
    {
    case (0):
        // linear fog
        incrementLinear();
        break;
    default:
        return;
    }

    // rotate camera

    m_stepCounter++;
}