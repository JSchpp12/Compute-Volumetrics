#pragma once

#include "controller/SimulationController.hpp"
#include "controller/detail/SimulationBoundsFile.hpp"

#include <starlight/virtual/StarCamera.hpp>

#include <future>

class CircleCameraController : public ISimulationController
{
  public:
    CircleCameraController(std::shared_ptr<star::StarCamera> camera);

    virtual void init(star::core::device::DeviceContext &context) override; 

    virtual void frameUpdate(star::core::device::DeviceContext &context) override; 

  private:
    int fogTypeTracker; 
    uint8_t m_rotationCounter; 
    std::future<controller::SimulationBounds> m_loadedBounds; 

    std::shared_ptr<star::StarCamera> m_camera = nullptr;
};