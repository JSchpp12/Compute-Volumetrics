#pragma once

#include <starlight/core/device/IStartupDeviceRequirementsProvider.hpp>

/// Application-specific device requirements
class VolumetricsDeviceRequirementsProvider final : public star::core::device::IStartupDeviceRequirementsProvider
{
  public:
    VolumetricsDeviceRequirementsProvider() = default;
    ~VolumetricsDeviceRequirementsProvider() override = default;

    star::core::device::DeviceRequirements getRequirements() const override;
};
