#pragma once

#include "controller/SimulationController.hpp"
#include "controller/detail/simulation_bounds_file/SimulationSteps.hpp"
#include "Volume.hpp"

#include <starlight/virtual/StarCamera.hpp>

#include <future>

class CircleCameraController : public ISimulationController
{
  public:
    CircleCameraController(std::shared_ptr<star::StarCamera> camera, std::shared_ptr<Volume> volume);

    virtual void init(star::core::device::DeviceContext &context) override;

    virtual void frameUpdate(star::core::device::DeviceContext &context) override;

    virtual bool isDone() const override; 

  private:
    int m_fogTypeTracker = 0;
    int m_rotationCounter = 0;
    int m_stepCounter = 0; 
    controller::simulation_bounds_file::SimulationSteps m_loadedSteps;
    std::future<controller::simulation_bounds_file::SimulationSteps> m_loadedInfo;
    std::shared_ptr<star::StarCamera> m_camera = nullptr;
    std::shared_ptr<Volume> m_volume = nullptr; 

    void switchFogType(int newType); 
    void submitReadCmd(star::core::device::DeviceContext &context, const std::string &path);
    void incrementLinear();
};