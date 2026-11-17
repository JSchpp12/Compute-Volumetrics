#pragma once

#include "service/detail/simulation_controller/SimulationData.hpp"

#include <future>
#include <utility>

namespace service::simulation_controller
{
/// @brief Owns an in-flight controller-file read and hands out the loaded data once the IO worker is done.
class PendingSimulationData
{
  public:
    PendingSimulationData() = default;
    explicit PendingSimulationData(std::future<SimulationData> future) : m_future(std::move(future))
    {
    }

    /// @brief Blocks until the IO worker satisfies the read. A second call throws.
    SimulationData get()
    {
        return m_future.get();
    }

  private:
    std::future<SimulationData> m_future;
};
} // namespace service::simulation_controller
