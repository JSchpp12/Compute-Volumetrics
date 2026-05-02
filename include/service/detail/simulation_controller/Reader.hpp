#pragma once

#include "service/detail/simulation_controller/SimulationData.hpp"

#include <future>
#include <string>

namespace service::simulation_controller
{
class Reader
{
  public:
    Reader() = default;
    int operator()(const std::filesystem::path &filePath);

    std::future<SimulationData> getFuture()
    {
        return m_loadedBounds.get_future();
    }

  private:
    std::promise<SimulationData> m_loadedBounds;
};
} // namespace service::simulation_controller