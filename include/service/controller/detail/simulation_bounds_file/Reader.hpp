#pragma once

#include "service/controller/detail/simulation_bounds_file/SimulationBounds.hpp"
#include "service/controller/detail/simulation_bounds_file/SimulationSteps.hpp"

#include <future>
#include <string>

namespace controller::simulation_bounds_file
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
} // namespace controller::simulation_bounds_file