#pragma once

#include "FogInfo.hpp"

#include <string>
#include <future>

namespace controller
{
struct SimulationBounds
{
    FogInfo start;
    FogInfo stop;
};

class SimulationBoundsFile
{
  public:
    SimulationBoundsFile() = default; 
    explicit SimulationBoundsFile(std::string filePath);
    int operator()(const std::string &filePath); 

    std::future<SimulationBounds> getFuture()
    {
        return m_loadedBounds.get_future();
    }
  private:
    std::string m_filePath;
    std::promise<SimulationBounds> m_loadedBounds; 

    // std::shared_ptr<std::future<SimulationBounds>> m_loadedBounds;
};
} // namespace controller