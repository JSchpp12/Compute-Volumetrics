#include "service/SimulationController.hpp"

#include "MathHelpers.hpp"
#include "TerrainChunk.hpp"
#include "TerrainInfoFile.hpp"
#include "TerrainShapeInfo.hpp"
#include "TerrainShapeInfoLoader.hpp"
#include "service/detail/simulation_controller/Reader.hpp"
#include "service/detail/simulation_controller/Writer.hpp"

#include <starlight/command/FileIO/ReadFromFile.hpp>
#include <starlight/command/FileIO/WriteToFile.hpp>

#include <star_common/helper/PathHelpers.hpp>

#include <filesystem>

SimulationControllerService::SimulationControllerService()
    : m_loadedSteps(), m_loadedInfo(), m_worldHeightAtCenterTerrain(0.0), m_fogTypeTracker(0), m_stepCounter(0),
      m_onTriggerUpdate(*this), m_onListenForDone(*this)
{
}

SimulationControllerService::SimulationControllerService(std::shared_ptr<bool> doneFlag)
    : m_loadedSteps(), m_loadedInfo(), m_worldHeightAtCenterTerrain(0.0), m_fogTypeTracker(0), m_stepCounter(0),
      m_onTriggerUpdate(*this), m_onListenForDone(*this), m_doneFlag(std::move(doneFlag))
{
}

SimulationControllerService::SimulationControllerService(SimulationControllerService &&other)
    : m_loadedSteps(std::move(other.m_loadedSteps)), m_loadedInfo(std::move(other.m_loadedInfo)),
      m_worldHeightAtCenterTerrain(std::move(other.m_worldHeightAtCenterTerrain)),
      m_fogTypeTracker(std::move(other.m_fogTypeTracker)), m_stepCounter(std::move(other.m_stepCounter)),
      m_onTriggerUpdate(*this), m_onListenForDone(*this), m_cmd(other.m_cmd), m_doneFlag(other.m_doneFlag)
{
    if (m_cmd != nullptr)
    {
        other.cleanupListeners(*m_cmd);
        initListeners(*m_cmd);
    }
}

SimulationControllerService &SimulationControllerService::operator=(SimulationControllerService &&other)
{
    if (this != &other)
    {
        m_loadedSteps = std::move(other.m_loadedSteps);
        m_loadedInfo = std::move(other.m_loadedInfo);
        m_worldHeightAtCenterTerrain = std::move(other.m_worldHeightAtCenterTerrain);
        m_fogTypeTracker = std::move(other.m_fogTypeTracker);
        m_stepCounter = std::move(other.m_stepCounter);
        m_cmd = other.m_cmd;
        m_doneFlag = other.m_doneFlag;

        if (m_cmd != nullptr)
        {
            other.cleanupListeners(*m_cmd);
            initListeners(*m_cmd);
        }
    }

    return *this;
}

void SimulationControllerService::initListeners(star::core::CommandBus &cmdBus)
{
    m_onTriggerUpdate.init(cmdBus);
    m_onListenForDone.init(cmdBus);
}

void SimulationControllerService::cleanupListeners(star::core::CommandBus &cmdBus)
{
    m_onTriggerUpdate.cleanup(cmdBus);
    m_onListenForDone.cleanup(cmdBus);
}

void SimulationControllerService::submitReadCmd(star::core::CommandBus &cmdBus, const std::string &path)
{
    const auto type = cmdBus.getRegistry().getType(star::command::file_io::ReadFromFile::GetUniqueTypeName());

    service::simulation_controller::Reader reader{};
    m_loadedInfo = reader.getFuture();
    star::job::tasks::io::ReadPayload payload{path, std::move(reader)};

    auto readTask = star::job::tasks::io::CreateReadTask(std::move(payload));
    auto readCmd = star::command::file_io::ReadFromFile(std::move(readTask));

    cmdBus.submit(readCmd);
}

static void WriteDefaultControllerInfo(star::core::CommandBus &cmdBus, const std::string &path)
{
    auto writePayload = star::job::tasks::io::WritePayload{path, service::simulation_controller::Writer{}};
    auto writeCmd = star::command::file_io::WriteToFile(star::job::tasks::io::CreateWriteTask(std::move(writePayload)));
    cmdBus.submit(writeCmd);
}

void SimulationControllerService::onTriggerUpdate(sim_controller::TriggerUpdate &cmd)
{
    updateSim(cmd.volume, cmd.camera);
}

void SimulationControllerService::setInitParameters(star::service::InitParameters &params)
{
    m_cmd = &params.commandBus;
}

void SimulationControllerService::shutdown()
{
    assert(m_cmd != nullptr);
    cleanupListeners(*m_cmd);
}

void SimulationControllerService::init()
{
    assert(m_cmd && "Command bus should have been registered during setInitParams");
    const auto filePath = std::filesystem::path(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)) /
                          "SimulationController.json";

    initListeners(*m_cmd);

    if (std::filesystem::exists(filePath))
    {
        submitReadCmd(*m_cmd, filePath.string());
    }
    else
    {
        std::promise<service::simulation_controller::SimulationData> promise;
        m_loadedInfo = promise.get_future();
        promise.set_value({});

        WriteDefaultControllerInfo(*m_cmd, filePath.string());
    }

    m_fogTypeTracker = 0;
    m_stepCounter = 1000000;
}

void SimulationControllerService::switchFogType(int newType, Volume &volume, star::StarCamera &camera)
{
    auto type = Fog::Type::linear;

    switch (newType)
    {
    case (1):
        type = Fog::Type::marched_homogenous;
        break;
    case (2):
        type = Fog::Type::linear;
        break;
    case (3):
        type = Fog::Type::exp;
        break;
    default:
        std::cout << "Unknown fog type";
    }

    volume.getRenderer().setFogInfo(m_loadedSteps.start);
    volume.getRenderer().setFogType(type);
}

void SimulationControllerService::onCheckIfDone(sim_controller::CheckIfDone &cmd) const
{
    cmd.getReply().set(isDone());
}

void SimulationControllerService::incrementExp(Volume &volume) const
{
    volume.getRenderer().getFogInfo().expFogInfo.density += m_loadedSteps.fogInfoChanges.expFogInfo.density;
}

void SimulationControllerService::incrementLinear(Volume &volume) const
{
    volume.getRenderer().getFogInfo().linearInfo.farDist += m_loadedSteps.fogInfoChanges.linearInfo.farDist;
    volume.getRenderer().getFogInfo().linearInfo.nearDist += m_loadedSteps.fogInfoChanges.linearInfo.nearDist;
}

void SimulationControllerService::incrementMarched(Volume &volume) const
{
    volume.getRenderer().getFogInfo().homogenousInfo.maxNumSteps +=
        m_loadedSteps.fogInfoChanges.homogenousInfo.maxNumSteps;
    volume.getRenderer().getFogInfo().marchedInfo.defaultDensity +=
        m_loadedSteps.fogInfoChanges.marchedInfo.defaultDensity;
    volume.getRenderer().getFogInfo().marchedInfo.stepSizeDist += m_loadedSteps.fogInfoChanges.marchedInfo.stepSizeDist;
    volume.getRenderer().getFogInfo().marchedInfo.stepSizeDist_light +=
        m_loadedSteps.fogInfoChanges.marchedInfo.stepSizeDist_light;

    {
        const auto value = volume.getRenderer().getFogInfo().marchedInfo.getLightPropertyDirG();
        const auto inc = m_loadedSteps.fogInfoChanges.marchedInfo.getLightPropertyDirG();
        volume.getRenderer().getFogInfo().marchedInfo.setLightPropertyDirG(value + inc);
    }
    {
        const auto value = volume.getRenderer().getFogInfo().marchedInfo.getSigmaAbsorption();
        const auto inc = m_loadedSteps.fogInfoChanges.marchedInfo.getSigmaAbsorption();
        volume.getRenderer().getFogInfo().marchedInfo.setSigmaAbsorption(value + inc);
    }
    {
        const auto value = volume.getRenderer().getFogInfo().marchedInfo.getSigmaScattering();
        const auto inc = volume.getRenderer().getFogInfo().marchedInfo.getSigmaScattering();
        volume.getRenderer().getFogInfo().marchedInfo.setSigmaScattering(value + inc);
    }
}

bool SimulationControllerService::isDone() const
{
    return m_stepCounter == m_loadedSteps.numSteps && m_loadedController.isDone() && m_fogTypeTracker == 3;
}

void SimulationControllerService::updateSim(Volume &volume, star::StarCamera &camera)
{
    // check if the bounds are loaded
    if (!m_loadedSteps)
    {
        try
        {
            auto data = m_loadedInfo.get();
            m_loadedSteps = std::move(data.steps);
            m_loadedController = std::move(data.cameraController);
        }
        catch (...)
        {
            STAR_THROW("Attempted to call get on future that has already been consumed. This signifies that the json "
                       "file is not valid");
        }
    }

    if (!m_isCameraAtHeight)
    {
        auto camPos = camera.getPosition();

        // cam pos starts at ground level
        camPos.y += 2;
        camera.setPosition(camPos);
        m_isCameraAtHeight = true;
        switchFogType(1, volume, camera); 
        m_fogTypeTracker = 1; 
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
            incrementMarched(volume);
            break;
        case (2):
            incrementLinear(volume);
            break;
        case (3):
            incrementExp(volume);
            break;
        default:
            return;
        }
        m_stepCounter++;
    }
    else if (!m_loadedController.isDone())
    {
        volume.getRenderer().setFogInfo(m_loadedSteps.start);
        m_loadedController.tick(camera);
        m_stepCounter = 1;
    }
    else if (m_fogTypeTracker < 3)
    {
        // switch to next fog type
        m_fogTypeTracker++;
        //  set next fog type -- circle done
        switchFogType(m_fogTypeTracker, volume, camera);
        m_loadedController.reset(camera);
        m_stepCounter = 1;
    }

    if (isDone() && m_doneFlag)
    {
        *m_doneFlag = true;
    }
}