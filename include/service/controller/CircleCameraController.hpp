#pragma once

#include "command/sim_controller/TriggerUpdate.hpp"
#include "service/controller/detail/simulation_bounds_file/SimulationSteps.hpp"

#include <starlight/policy/command/ListenFor.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <future>

template <typename T>
using ListenForTriggerUpdate =
    star::policy::command::ListenFor<T, sim_controller::TriggerUpdate, sim_controller::trigger_update::GetTypeName,
                                      &T::onTriggerUpdate>;

class CircleCameraController
{
  public:
    CircleCameraController(); 
    explicit CircleCameraController(std::shared_ptr<bool> doneFlag); 
    CircleCameraController(const CircleCameraController &) = delete;
    CircleCameraController &operator=(const CircleCameraController &) = delete;
    CircleCameraController(CircleCameraController &&); 
    CircleCameraController &operator=(CircleCameraController &&); 

    void init();

    void setInitParameters(star::service::InitParameters &params);

    void negotiateWorkers(star::core::WorkerPool &pool, star::job::TaskManager &tm)
    {
    }

    void shutdown();

    void onTriggerUpdate(sim_controller::TriggerUpdate &cmd);

    bool isDone() const;

  private:
    controller::simulation_bounds_file::SimulationSteps m_loadedSteps;
    std::future<controller::simulation_bounds_file::SimulationSteps> m_loadedInfo;
    double m_worldHeightAtCenterTerrain; 
    int m_fogTypeTracker = 0;
    int m_rotationCounter = 0;
    int m_stepCounter = 0;
    ListenForTriggerUpdate<CircleCameraController> m_onTriggerUpdate; 
    star::core::CommandBus *m_cmd = nullptr;
    std::shared_ptr<bool> m_doneFlag = nullptr; 
    bool m_isCameraAtHeight = false; 

    void switchFogType(int newType, Volume &volume, star::StarCamera &camera);
    void submitReadCmd(star::core::CommandBus &cmdBus, const std::string &path);
    void incrementLinear(Volume &volume) const;
    void incrementExp(Volume &volume) const;
    void incrementMarched(Volume &volume) const; 
    void updateSim(Volume &volume, star::StarCamera &camera);
    void initListeners(star::core::CommandBus &cmdBus); 
    void cleanupListeners(star::core::CommandBus &cmdBus); 
};