#pragma once

#include "command/sim_controller/TriggerUpdate.hpp"
#include "command/sim_controller/CheckIfDone.hpp"
#include "service/detail/simulation_controller/SimulationData.hpp"
#include "service/detail/simulation_controller/SimulationSteps.hpp"
#include "service/detail/simulation_controller/FogEnabler.hpp"

#include <starlight/policy/command/ListenFor.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <optional>
#include <variant>
#include <future>

template <typename T>
using ListenForTriggerUpdate =
    star::policy::command::ListenFor<T, sim_controller::TriggerUpdate, sim_controller::trigger_update::GetTypeName,
                                      &T::onTriggerUpdate>;

template <typename T>
using ListenForCheckIfDone =
    star::policy::command::ListenFor<T, sim_controller::CheckIfDone, sim_controller::check_if_done::GetTypeName,
                                     &T::onCheckIfDone>;
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

    bool isDone() const;

  private:
    service::simulation_controller::SimulationSteps m_loadedSteps;
    service::simulation_controller::CameraController m_loadedController;
    service::simulation_controller::FogEnabler m_fogEnabledStatus; 
    std::future<service::simulation_controller::SimulationData> m_loadedInfo;
    std::string m_controllerFilePath; 
    double m_worldHeightAtCenterTerrain; 
    int m_stepCounter = 0;
    Fog::Type m_fogTypeTracker = Fog::Type::sNone; 
    ListenForTriggerUpdate<SimulationControllerService> m_onTriggerUpdate; 
    ListenForCheckIfDone<SimulationControllerService> m_onListenForDone; 
    star::core::CommandBus *m_cmd = nullptr;
    std::shared_ptr<bool> m_doneFlag = nullptr; 
    bool m_isPrimed = false; 

    void switchFogType(Fog::Type newType, Volume &volume, star::StarCamera &camera);
    void submitReadCmd(star::core::CommandBus &cmdBus, const std::string &path);
    void incrementLinear(Volume &volume, float t) const;
    void incrementExp(Volume &volume, float t) const;
    void incrementMarched(Volume &volume, float t) const; 
    void updateSim(Volume &volume, star::StarCamera &camera);
    void initListeners(star::core::CommandBus &cmdBus); 
    void cleanupListeners(star::core::CommandBus &cmdBus); 
    Fog::Type selectNextFogType() const; 
};