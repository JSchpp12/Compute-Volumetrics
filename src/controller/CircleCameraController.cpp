#include "controller/CircleCameraController.hpp"

#include "controller/detail/simulation_bounds_file/Reader.hpp"
#include "controller/detail/simulation_bounds_file/Writer.hpp"

#include <starlight/command/FileIO/ReadFromFile.hpp>
#include <starlight/command/FileIO/WriteToFile.hpp>

#include <star_common/helper/PathHelpers.hpp>

#include <filesystem>

void CircleCameraController::submitReadCmd(star::core::CommandBus &cmdBus, const std::string &path)
{
    const auto type = cmdBus.getRegistry().getType(star::command::file_io::ReadFromFile::GetUniqueTypeName()); 

    controller::simulation_bounds_file::Reader reader{};
    m_loadedInfo = reader.getFuture();
    star::job::tasks::io::ReadPayload payload{path, std::move(reader)};

    auto readTask = star::job::tasks::io::CreateReadTask(std::move(payload));
    auto readCmd = star::command::file_io::ReadFromFile(std::move(readTask));

    cmdBus.submit(readCmd); 
}

static void WriteDefaultControllerInfo(star::core::CommandBus &cmdBus, const std::string &path)
{
    auto writePayload = star::job::tasks::io::WritePayload{path, controller::simulation_bounds_file::Writer{}};
    auto writeCmd = star::command::file_io::WriteToFile(star::job::tasks::io::CreateWriteTask(std::move(writePayload)));
    cmdBus.submit(writeCmd);
}

void CircleCameraController::setInitParameters(star::service::InitParameters &params)
{
    m_cmd = &params.commandBus;
}

void CircleCameraController::init()
{
    assert(m_cmd && "Command bus should have been registered during setInitParams"); 

    const auto filePath = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) /
                          "SimulationController.json";

    if (std::filesystem::exists(filePath))
    {
        submitReadCmd(*m_cmd, filePath.string());
    }
    else
    {
        std::promise<controller::simulation_bounds_file::SimulationSteps> promise;
        m_loadedInfo = promise.get_future();
        promise.set_value({});

        WriteDefaultControllerInfo(*m_cmd, filePath.string());
    }

    m_rotationCounter = 361;
    m_stepCounter = 1000000;
}

void CircleCameraController::switchFogType(int newType, Volume &volume, star::StarCamera &camera) const
{
    auto type = Fog::Type::linear;

    switch (newType)
    {
    case (1):
        type = Fog::Type::linear;
        break;
    case (2):
        type = Fog::Type::exp;
        break;
    case (3):
        type = Fog::Type::marched_homogenous;
        break;
    default:
        std::cout << "Unknown fog type";
    }

    volume.getRenderer().setFogInfo(m_loadedSteps.start);
    volume.getRenderer().setFogType(type);
    camera.setForwardVector(glm::vec3{1.0, 0.0, 0.0});
}

void CircleCameraController::incrementLinear(Volume &volume) const
{
    volume.getRenderer().getFogInfo().linearInfo.farDist += m_loadedSteps.fogInfoChanges.linearInfo.farDist;
    volume.getRenderer().getFogInfo().linearInfo.nearDist += m_loadedSteps.fogInfoChanges.linearInfo.nearDist;
}

bool CircleCameraController::isDone() const
{
    return m_stepCounter == m_loadedSteps.numSteps && m_rotationCounter == 360 && m_fogTypeTracker == 1;
}

void CircleCameraController::updateSim(Volume &volume, star::StarCamera &camera)
{
    // check if the bounds are loaded
    if (!m_loadedSteps)
    {
        try
        {
            m_loadedSteps = m_loadedInfo.get();
        }
        catch (...)
        {
            STAR_THROW("Attempted to call get on future that has already been consumed. This signifies that the json "
                       "file is not valid");
        }
    }

    if (isDone())
    {
        return;
    }

    if (m_stepCounter < m_loadedSteps.numSteps)
    {
        switch (m_fogTypeTracker)
        {
        case (1):
            // linear fog
            incrementLinear(volume);
            break;
        default:
            return;
        }
        m_stepCounter++;
    }
    else if (m_rotationCounter < 360)
    {
        volume.getRenderer().setFogInfo(m_loadedSteps.start);
        camera.rotateRelative(star::Type::Axis::y, 1.0);

        m_rotationCounter++;
        m_stepCounter = 0;
    }
    else if (m_fogTypeTracker < 1)
    {
        // switch to next fog type
        m_fogTypeTracker++;
        //  set next fog type -- circle done
        switchFogType(m_fogTypeTracker, volume, camera);

        m_rotationCounter = 0;
        m_stepCounter = 0;
    }
}