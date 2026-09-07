#include "policy/VolumetricsDeviceRequirementsProvider.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vulkan/vulkan.hpp>

TEST(VolumetricsDeviceRequirementsProvider, RequestsShaderFloat64)
{
    VolumetricsDeviceRequirementsProvider provider;
    const auto requirements = provider.getRequirements();

    ASSERT_EQ(requirements.physicalDeviceFeatureRequests.size(), 1);
    EXPECT_EQ(std::string{requirements.physicalDeviceFeatureRequests[0].name}, "shaderFloat64");

    vk::PhysicalDeviceFeatures features{};
    features.*requirements.physicalDeviceFeatureRequests[0].feature = VK_TRUE;
    EXPECT_EQ(features.shaderFloat64, VK_TRUE);
}

TEST(VolumetricsDeviceRequirementsProvider, HasNoAdditionalRequiredExtensions)
{
    VolumetricsDeviceRequirementsProvider provider;
    const auto requirements = provider.getRequirements();

    EXPECT_TRUE(requirements.requiredDeviceExtensions.empty());
}
