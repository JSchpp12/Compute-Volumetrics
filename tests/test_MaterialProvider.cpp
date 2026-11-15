#include <gtest/gtest.h>

#include <starlight/material/DefaultColorProvider.hpp>
#include <starlight/material/InstanceColorMaterial.hpp>
#include <starlight/material/MaterialProvider.hpp>

#include <memory>
#include <vector>

namespace
{
std::vector<star::Color> MakeColors()
{
    return {star::Color{1.0f, 0.0f, 0.0f, 1.0f}, star::Color{0.0f, 1.0f, 0.0f, 1.0f}};
}

std::unique_ptr<star::material::InstanceColorMaterial> MakeInstanceColorMaterial()
{
    return std::make_unique<star::material::InstanceColorMaterial>(
        std::make_unique<star::material::DefaultColorProvider>(MakeColors()));
}
} // namespace

// MaterialProvider must expose exactly the metadata of the material it will produce; otherwise the phase-built
// pipeline layout and the descriptor pool sizing drift from the allocated material sets.
TEST(MaterialProvider, MetadataMatchesWrappedMaterial)
{
    constexpr int numFramesInFlight = 3;

    auto direct = MakeInstanceColorMaterial();
    const auto directRequests = direct->getDescriptorRequests(numFramesInFlight);

    star::StarDescriptorSetLayout::Builder directBuilder;
    direct->addDescriptorSetLayoutsTo(directBuilder);
    const auto directLayout = directBuilder.build();

    star::MaterialProvider provider(MakeInstanceColorMaterial());

    EXPECT_EQ(provider.getDescriptorRequests(numFramesInFlight), directRequests);

    star::StarDescriptorSetLayout::Builder providerBuilder;
    provider.addDescriptorSetLayoutsTo(providerBuilder);
    const auto providerLayout = providerBuilder.build();

    const auto &directBindings = directLayout->getBindings();
    const auto &providerBindings = providerLayout->getBindings();

    ASSERT_EQ(providerBindings.size(), directBindings.size());
    for (const auto &[binding, expected] : directBindings)
    {
        const auto it = providerBindings.find(binding);
        ASSERT_NE(it, providerBindings.end());
        EXPECT_EQ(it->second.descriptorType, expected.descriptorType);
        EXPECT_EQ(it->second.descriptorCount, expected.descriptorCount);
        EXPECT_EQ(it->second.stageFlags, expected.stageFlags);
    }
}
