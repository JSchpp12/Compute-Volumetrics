#pragma once

#include "service/detail/simulation_controller/SimulationBounds.hpp"

namespace service::simulation_controller
{
class Writer
{
  public:
    Writer() = default;
    explicit Writer(SimulationBounds bounds) : m_bounds(std::move(bounds))
    {
    }
    int operator()(const std::string &);

  private:
    SimulationBounds m_bounds;
};
} // namespace service::simulation_controller