#pragma once

#include "service/detail/simulation_controller/SimulationBounds.hpp"
#include "service/detail/simulation_controller/SimulationSteps.hpp"

#include <future>
#include <string>

namespace service::simulation_controller
{
class Reader
{
  public:
    Reader() = default;
    int operator()(const std::string &filePath);

    std::future<SimulationSteps> getFuture()
    {
        return m_loadedBounds.get_future();
    }

  private:
    std::promise<SimulationSteps> m_loadedBounds;
};
} // namespace service::simulation_controller