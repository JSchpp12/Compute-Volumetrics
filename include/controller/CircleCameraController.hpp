#pragma once

#include "command/sim_controller/TriggerUpdate.hpp"
#include "controller/detail/simulation_bounds_file/SimulationSteps.hpp"

#include <starlight/virtual/StarCamera.hpp>

#include <future>

class CircleCameraController
{
  public:
    void init();

    void setInitParameters(star::service::InitParameters &params);

    void negotiateWorkers(star::core::WorkerPool &pool, star::job::TaskManager &tm)
    {
    }

    void shutdown(); 

    void onTriggerUpdate(); 

    bool isDone() const;

  private:
    controller::simulation_bounds_file::SimulationSteps m_loadedSteps;
    std::future<controller::simulation_bounds_file::SimulationSteps> m_loadedInfo;
    int m_fogTypeTracker = 0;
    int m_rotationCounter = 0;
    int m_stepCounter = 0;
    star::core::CommandBus *m_cmd = nullptr; 

    void switchFogType(int newType, Volume &volume, star::StarCamera &camera) const;
    void submitReadCmd(star::core::CommandBus &cmdBus, const std::string &path);
    void incrementLinear(Volume &volume) const;
    void updateSim(Volume &volume, star::StarCamera &camera);
};