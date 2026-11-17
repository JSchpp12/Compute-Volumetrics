#pragma once

#include "service/detail/simulation_controller/PendingSimulationData.hpp"
#include "service/detail/simulation_controller/SimulationData.hpp"

#include <filesystem>
#include <utility>
#include <variant>

namespace star::core
{
class CommandBus;
} // namespace star::core

namespace service::simulation_controller
{
/// @brief Resolves the active simulation's data on first use.
/// @details Build one through Builder: building submits the controller-file read to the command bus and returns a
/// source holding its result, so owners do not construct or dispatch the file IO command themselves.
class SimulationDataSource
{
  public:
    class Builder
    {
      public:
        explicit Builder(star::core::CommandBus &cmdBus) : m_cmdBus(&cmdBus)
        {
        }

        Builder &setPath(std::filesystem::path path)
        {
            m_path = std::move(path);
            return *this;
        }

        /// @brief Submits the read and returns the source holding its result.
        SimulationDataSource build();

      private:
        star::core::CommandBus *m_cmdBus{nullptr};
        std::filesystem::path m_path;
    };

    SimulationDataSource() = default;
    explicit SimulationDataSource(PendingSimulationData pending) : m_data(std::move(pending))
    {
    }
    SimulationDataSource(const SimulationDataSource &) = delete;
    SimulationDataSource &operator=(const SimulationDataSource &) = delete;
    SimulationDataSource(SimulationDataSource &&) = default;
    SimulationDataSource &operator=(SimulationDataSource &&) = default;

    /// @brief The loaded data, consuming the pending read if needed. May block on the IO worker.
    SimulationData &get();
    /// @brief The loaded data. Only valid once get() has resolved the read.
    const SimulationData &get() const;

    [[nodiscard]] bool isLoaded() const
    {
        return std::holds_alternative<SimulationData>(m_data);
    }

  private:
    std::variant<PendingSimulationData, SimulationData> m_data;
};
} // namespace service::simulation_controller
