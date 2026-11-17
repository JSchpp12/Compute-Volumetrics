#pragma once

#include "command/sim_controller/CheckIfDone.hpp"
#include "command/sim_controller/GetSimulationCameraInfo.hpp"
#include "command/sim_controller/TriggerUpdate.hpp"
#include "service/detail/simulation_controller/FogEnabler.hpp"
#include "service/detail/simulation_controller/SimulationDataSource.hpp"

#include <starlight/policy/command/ListenFor.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <memory>
#include <optional>
#include <string>

template <typename T>
using ListenForTriggerUpdate =
    star::policy::command::ListenFor<T, sim_controller::TriggerUpdate, sim_controller::trigger_update::GetTypeName,
                                     &T::onTriggerUpdate>;

template <typename T>
using ListenForCheckIfDone =
    star::policy::command::ListenFor<T, sim_controller::CheckIfDone, sim_controller::check_if_done::GetTypeName,
                                     &T::onCheckIfDone>;

template <typename T>
using ListenForCameraInfo = star::policy::command::ListenFor<T, sim_controller::GetSimulationCameraInfo,
                                                             sim_controller::get_simulation_camera_info::GetTypeName,
                                                             &T::onGetSimulationCameraInfo>;

class SimulationControllerService
{
  public:
    explicit SimulationControllerService(std::string controllerFilePath);
    explicit SimulationControllerService(std::string controllerFilePath, std::shared_ptr<bool> doneFlag);
    SimulationControllerService(const SimulationControllerService &) = delete;
    SimulationControllerService &operator=(const SimulationControllerService &) = delete;
    SimulationControllerService(SimulationControllerService &&);
    SimulationControllerService &operator=(SimulationControllerService &&);

    void init();

    void setInitParameters(star::service::InitParameters &params);

    void negotiateWorkers(star::core::WorkerPool &pool, star::job::TaskManager &tm)
    {
    }

    void shutdown();

    void onTriggerUpdate(sim_controller::TriggerUpdate &cmd);

    void onCheckIfDone(sim_controller::CheckIfDone &cmd) const;

    void onGetSimulationCameraInfo(sim_controller::GetSimulationCameraInfo &cmd);

    bool isDone() const;

  private:
    std::string m_controllerFilePath;
    double m_worldHeightAtCenterTerrain;
    int m_stepCounter = 0;
    Fog::Type m_fogTypeTracker = Fog::Type::sNone;
    service::simulation_controller::SimulationDataSource m_activeSimulation;
    bool m_isPrimed = false;
    bool m_cameraPositioned = false;
    star::core::CommandBus *m_cmd = nullptr;
    std::shared_ptr<bool> m_doneFlag = nullptr;
    ListenForTriggerUpdate<SimulationControllerService> m_onTriggerUpdate;
    ListenForCheckIfDone<SimulationControllerService> m_onListenForDone;
    ListenForCameraInfo<SimulationControllerService> m_onGetCameraInfo;

    std::optional<CameraSweepInfo> getCameraInfo();
    void switchFogType(Fog::Type newType, Volume &volume, star::StarCamera &camera);
    void incrementLinear(Volume &volume, float t) const;
    void incrementExp(Volume &volume, float t) const;
    void incrementMarched(Volume &volume, float t) const;
    sim_controller::UpdateStatus updateSim(Volume &volume, star::StarCamera &camera);
    void initListeners(star::core::CommandBus &cmdBus);
    void cleanupListeners(star::core::CommandBus &cmdBus);
    Fog::Type selectNextFogType() const;
    void applyInitialCamera(star::StarCamera &cam);
};