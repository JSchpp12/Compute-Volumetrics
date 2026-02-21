#pragma once

#include "controller/detail/simulation_bounds_file/SimulationBounds.hpp"

namespace controller::simulation_bounds_file
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
} // namespace controller::simulation_bounds_file