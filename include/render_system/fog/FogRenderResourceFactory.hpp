#pragma once

#include "render_system/fog/PipelineVariant.hpp"
#include "render_system/fog/policies/TransmittanceResourceResolutionPolicy.hpp"

#include <star_common/Handle.hpp>

#include <utility>
#include <vector>

namespace star::core
{
class CommandBus;
}
namespace star::core::device
{
class DeviceContext;
}
namespace star::core::renderer
{
class FrameData;
}

namespace render_system::fog
{
/// @brief Owns the policies that describe the render resources the fog/volume passes use, and creates those
/// resources from them.
///
/// One instance is built by the owner of the volume and shared with every provider that needs the same description,
/// so the transmittance map, its noise texture and the pipelines that consume them cannot drift apart.
class FogRenderResourceFactory
{
  public:
    /// Roles the transmittance resources were registered under in the frame data.
    struct TransmittanceResources
    {
        star::Handle mapRole;
        star::Handle mapSampledRole;
        star::Handle noiseRole;
    };

    explicit FogRenderResourceFactory(policies::TransmittanceResourceResolutionPolicy resourcePolicies)
        : resourcePolicies(std::move(resourcePolicies))
    {
    }

    /// @brief Creates the transmittance map (one per frame in flight) and the per-column noise texture, and
    /// registers both in `frameData`.
    TransmittanceResources createTransmittanceResources(star::core::device::DeviceContext &context,
                                                        star::core::renderer::FrameData &frameData) const;

    /// @brief Builds one transmittance precompute pipeline variant per entry in the plan's variant table, in table
    /// order. Every variant shares the same shader and differs only by its specialization constants.
    std::vector<PipelineVariant> transmittancePipelineVariants(star::core::CommandBus &commandBus) const;

    /// @brief Holds the per-resource policies describing the render resources. Set through the constructor and fixed
    /// for the lifetime of the factory; further policies join as more resource families join the fog pass.
    policies::TransmittanceResourceResolutionPolicy resourcePolicies;
};
} // namespace render_system::fog
