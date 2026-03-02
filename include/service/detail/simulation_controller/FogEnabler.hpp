#pragma once

#include "FogType.hpp"

namespace service::simulation_controller
{
class FogEnabler
{
  public:
    FogEnabler() = default;
    explicit FogEnabler(std::array<bool, static_cast<size_t>(Fog::Type::sCountOfNonDebugTypes)> enabledStatus)
        : m_enabledStatus(enabledStatus)
    {
    }

    void setEnabled(Fog::Type type);
    bool isEnabled(Fog::Type type) const;
    constexpr size_t GetSize()
    {
        return static_cast<size_t>(Fog::Type::sCountOfNonDebugTypes);
    }

    std::array<bool, Fog::Type::sCountOfNonDebugTypes> getEnabledStatus() const
    {
        return m_enabledStatus;
    }

  private:
    std::array<bool, static_cast<size_t>(Fog::Type::sCountOfNonDebugTypes)> m_enabledStatus =
        std::array<bool, static_cast<size_t>(Fog::Type::sCountOfNonDebugTypes)>();
};
} // namespace service::simulation_controller