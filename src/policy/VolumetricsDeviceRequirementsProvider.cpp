#include "policy/VolumetricsDeviceRequirementsProvider.hpp"

star::core::device::DeviceRequirements VolumetricsDeviceRequirementsProvider::getRequirements() const
{
    star::core::device::DeviceRequirements requirements;
    requirements.physicalDeviceFeatureRequests.push_back({&vk::PhysicalDeviceFeatures::shaderFloat64, "shaderFloat64"});
    return requirements;
}
