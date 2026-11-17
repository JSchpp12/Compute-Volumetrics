#include "service/SimulationController.hpp"

#include "service/detail/simulation_controller/CameraController.hpp"
#include "service/detail/simulation_controller/Writer.hpp"
#include "service/detail/simulation_controller/camera_controller/Circle.hpp"
#include "util/FieldOfView.hpp"

#include <starlight/command/FileIO/WriteToFile.hpp>

#include <star_common/helper/PathHelpers.hpp>

#include <filesystem>

SimulationControllerService::SimulationControllerService(std::string controllerFilePath)
    : m_controllerFilePath(std::move(controllerFilePath)), m_worldHeightAtCenterTerrain(0.0), m_stepCounter(0),
      m_fogTypeTracker(Fog::Type::sNone), m_onTriggerUpdate(*this), m_onListenForDone(*this), m_onGetCameraInfo(*this)
{
}

SimulationControllerService::SimulationControllerService(std::string controllerFilePath, std::shared_ptr<bool> doneFlag)
    : m_controllerFilePath(std::move(controllerFilePath)), m_worldHeightAtCenterTerrain(0.0), m_stepCounter(0),
      m_fogTypeTracker(Fog::Type::sNone), m_onTriggerUpdate(*this), m_onListenForDone(*this), m_onGetCameraInfo(*this),
      m_doneFlag(std::move(doneFlag))
{
}

SimulationControllerService::SimulationControllerService(SimulationControllerService &&other)
    : m_controllerFilePath(std::move(other.m_controllerFilePath)),
      m_worldHeightAtCenterTerrain(std::move(other.m_worldHeightAtCenterTerrain)),
      m_stepCounter(std::move(other.m_stepCounter)), m_fogTypeTracker(std::move(other.m_fogTypeTracker)),
      m_activeSimulation(std::move(other.m_activeSimulation)), m_isPrimed(other.m_isPrimed),
      m_cameraPositioned(other.m_cameraPositioned), m_cmd(other.m_cmd), m_doneFlag(other.m_doneFlag),
      m_onTriggerUpdate(*this), m_onListenForDone(*this), m_onGetCameraInfo(*this)
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
        m_controllerFilePath = std::move(other.m_controllerFilePath);
        m_worldHeightAtCenterTerrain = std::move(other.m_worldHeightAtCenterTerrain);
        m_stepCounter = std::move(other.m_stepCounter);
        m_fogTypeTracker = std::move(other.m_fogTypeTracker);
        m_activeSimulation = std::move(other.m_activeSimulation);
        m_isPrimed = other.m_isPrimed;
        m_cameraPositioned = other.m_cameraPositioned;
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
    m_onGetCameraInfo.init(cmdBus);
}

void SimulationControllerService::cleanupListeners(star::core::CommandBus &cmdBus)
{
    m_onTriggerUpdate.cleanup(cmdBus);
    m_onListenForDone.cleanup(cmdBus);
    m_onGetCameraInfo.cleanup(cmdBus);
}

static void WriteDefaultControllerInfo(star::core::CommandBus &cmdBus, const std::string &path)
{
    auto writePayload = star::job::tasks::io::WritePayload{path, service::simulation_controller::Writer{}};
    auto writeCmd = star::command::file_io::WriteToFile(star::job::tasks::io::CreateWriteTask(std::move(writePayload)));
    cmdBus.submit(writeCmd);
}

void SimulationControllerService::onTriggerUpdate(sim_controller::TriggerUpdate &cmd)
{
    cmd.getReply().set(updateSim(cmd.volume, cmd.camera));
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

    initListeners(*m_cmd);
    const auto filePath = std::filesystem::path(m_controllerFilePath);

    if (std::filesystem::exists(filePath))
    {
        m_activeSimulation =
            service::simulation_controller::SimulationDataSource::Builder{*m_cmd}.setPath(filePath).build();
    }
    else
    {
        STAR_THROW("Provided controller file path does not exists: " + filePath.string());
    }

    m_stepCounter = 1000000;
}

void SimulationControllerService::switchFogType(Fog::Type newType, Volume &volume, star::StarCamera &camera)
{
}

void SimulationControllerService::onCheckIfDone(sim_controller::CheckIfDone &cmd) const
{
    cmd.getReply().set(isDone());
}

void SimulationControllerService::onGetSimulationCameraInfo(sim_controller::GetSimulationCameraInfo &cmd)
{
    cmd.getReply().set(getCameraInfo());
}

std::optional<CameraSweepInfo> SimulationControllerService::getCameraInfo()
{
    // Resolves the controller-file read if it is still pending.
    const auto &simulation = m_activeSimulation.get();
    const auto *circle = simulation.cameraController.getCircleController();
    if (circle == nullptr)
    {
        return std::nullopt;
    }

    const int numCameraPositions = circle->getNumCameraPositions();
    const float rotationDegreesPerTick = circle->getRotationDegreesPerTick();
    const glm::vec3 startViewDirection = circle->getStartCameraDirection();

    return CameraSweepInfo{
        .startViewDirection = glm::dot(startViewDirection, startViewDirection) > 0.0f
                                  ? std::optional<glm::vec3>{startViewDirection}
                                  : std::nullopt,
        .startCameraPosition = simulation.startCameraPosition,
        .numCameraPositions = numCameraPositions,
        .rotationDegreesPerTick = rotationDegreesPerTick,
        .rotationSpanDegrees = util::GetRotationSpanDegrees(numCameraPositions, rotationDegreesPerTick),
    };
}

void SimulationControllerService::incrementExp(Volume &volume, float t) const
{
    const auto &steps = m_activeSimulation.get().steps;
    volume.getRenderer().getFogInfo().expFogInfo.density =
        steps.start.expFogInfo.density + t * steps.fogInfoChanges.expFogInfo.density;
}

void SimulationControllerService::incrementLinear(Volume &volume, float t) const
{
    const auto &steps = m_activeSimulation.get().steps;
    volume.getRenderer().getFogInfo().linearInfo.farDist =
        steps.start.linearInfo.farDist + t * steps.fogInfoChanges.linearInfo.farDist;
    volume.getRenderer().getFogInfo().linearInfo.nearDist =
        steps.start.linearInfo.nearDist + t * steps.fogInfoChanges.linearInfo.nearDist;
}

void SimulationControllerService::incrementMarched(Volume &volume, float t) const
{
    const auto &steps = m_activeSimulation.get().steps;
    volume.getRenderer().getFogInfo().marchedInfo.defaultDensity =
        steps.start.marchedInfo.defaultDensity + t * steps.fogInfoChanges.marchedInfo.defaultDensity;
    volume.getRenderer().getFogInfo().marchedInfo.stepSizeDist =
        steps.start.marchedInfo.stepSizeDist + t * steps.fogInfoChanges.marchedInfo.stepSizeDist;
    volume.getRenderer().getFogInfo().marchedInfo.stepSizeDist_light =
        steps.start.marchedInfo.stepSizeDist_light + t * steps.fogInfoChanges.marchedInfo.stepSizeDist_light;
    volume.getRenderer().getFogInfo().marchedInfo.setLightPropertyDirG(
        steps.start.marchedInfo.getLightPropertyDirG() + t * steps.fogInfoChanges.marchedInfo.getLightPropertyDirG());
    volume.getRenderer().getFogInfo().marchedInfo.setSigmaAbsorption(
        steps.start.marchedInfo.getSigmaAbsorption() + t * steps.fogInfoChanges.marchedInfo.getSigmaAbsorption());
    volume.getRenderer().getFogInfo().marchedInfo.setSigmaScattering(
        steps.start.marchedInfo.getSigmaScattering() + t * steps.fogInfoChanges.marchedInfo.getSigmaScattering());
    volume.getRenderer().getFogInfo().marchedInfo.setDensityMultiplier(
        steps.start.marchedInfo.getDensityMultiplier() + t * steps.fogInfoChanges.marchedInfo.getDensityMultiplier());
    volume.getRenderer().getFogInfo().marchedInfo.setColorTransparencyCutoff(
        steps.start.marchedInfo.getColorTransparencyCutoff() +
        t * steps.fogInfoChanges.marchedInfo.getColorTransparencyCutoff());
    volume.getRenderer().getFogInfo().marchedInfo.setDistanceTransparencyCutoff(
        steps.start.marchedInfo.getDistanceTransparencyCutoff() +
        t * steps.fogInfoChanges.marchedInfo.getDistanceTransparencyCutoff());
    volume.getRenderer().getFogInfo().marchedInfo.setLightExtinctionScale(
        steps.start.marchedInfo.getLightExtinctionScale() +
        t * steps.fogInfoChanges.marchedInfo.getLightExtinctionScale());
}

bool SimulationControllerService::isDone() const
{
    if (!m_activeSimulation.isLoaded())
    {
        return false;
    }

    const auto &simulation = m_activeSimulation.get();
    return m_stepCounter == simulation.steps.numSteps - 1 && simulation.cameraController.isDone().value() &&
           selectNextFogType() == Fog::Type::sCount;
}

sim_controller::UpdateStatus SimulationControllerService::updateSim(Volume &volume, star::StarCamera &camera)
{
    sim_controller::UpdateStatus status;

    if (!m_cameraPositioned)
    {
        applyInitialCamera(camera);
        m_cameraPositioned = true;
    }

    auto &simulation = m_activeSimulation.get();
    auto &controller = simulation.cameraController;

    if (!m_isPrimed)
    {
        m_fogTypeTracker = selectNextFogType();
        if (static_cast<Fog::Type>(m_fogTypeTracker) == Fog::Type::sCount)
        {
            STAR_THROW("Controller does not contain any valid fog types");
        }

        volume.getRenderer().setFogType(m_fogTypeTracker);
        controller.reset(camera);
        m_stepCounter = 0;
        m_isPrimed = true;

        status.cameraLocation = true;
        status.cameraViewDirection = true;
    }
    else if (m_stepCounter == simulation.steps.numSteps - 1)
    {
        const bool camDone = controller.isDone().value();
        if (camDone)
        {
            m_fogTypeTracker = selectNextFogType();
            star::core::logging::info("Resetting camera controller");

            controller.reset(camera);

            if (m_fogTypeTracker != Fog::Type::sCount)
            {
                //  set next fog type -- circle done
                star::core::logging::info("Setting next fog type");

                volume.getRenderer().setFogType(m_fogTypeTracker);
                m_stepCounter = 0;
            }
        }
        else
        {
            star::core::logging::info("Incrementing camera controller");
            status.cameraViewDirection = true;
            controller.tick(camera);
            m_stepCounter = 0;
        }
    }
    else
    {
        m_stepCounter++;
    }

    float t = (m_stepCounter > 0) ? float(m_stepCounter) / float(simulation.steps.numSteps - 1) : 0.0f;
    status.fogParameters = true;
    switch (static_cast<Fog::Type>(m_fogTypeTracker))
    {
    case (Fog::Type::sMarched):
        incrementMarched(volume, t);
        break;
    case (Fog::Type::sLinear):
        incrementLinear(volume, t);
        break;
    case (Fog::Type::sExponential):
        incrementExp(volume, t);
        break;
    default:
        return status;
    }

    if (isDone() && m_doneFlag)
    {
        *m_doneFlag = true;
    }

    return status;
}

Fog::Type SimulationControllerService::selectNextFogType() const
{
    auto selected = Fog::Type::sCount;
    const size_t count = static_cast<size_t>(Fog::Type::sCountOfNonDebugTypes);
    const auto &fogStatus = m_activeSimulation.get().fogStatus;

    for (size_t i = static_cast<size_t>(m_fogTypeTracker) + 1; i < count; i++)
    {
        if (fogStatus.isEnabled(static_cast<Fog::Type>(i)))
        {
            selected = static_cast<Fog::Type>(i);
            break;
        }
    }

    return selected;
}

void SimulationControllerService::applyInitialCamera(star::StarCamera &cam)
{
    const auto &simulation = m_activeSimulation.get();

    if (simulation.startCameraPosition.has_value())
    {
        cam.setPosition(simulation.startCameraPosition.value());
        return;
    }

    const auto &camPos = cam.getPosition();
    // cam pos starts at ground level
    cam.setPosition({camPos.x, camPos.y + static_cast<float>(simulation.initialCameraHeightAboveGround), camPos.z});

    if (simulation.initialCameraHeightAboveGround <= 0)
    {
        STAR_THROW("Invalid initial camera height above ground provided. It must be greater than 0");
    }
}
