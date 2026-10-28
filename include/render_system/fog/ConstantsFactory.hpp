#pragma once

#include <star_common/Handle.hpp>
#include <starlight/wrappers/graphics/StarPipelineConstants.hpp>

#include <optional>

namespace star::command::pipeline
{
class CreatePipeline;
}

namespace render_system::fog
{
struct TransmittanceConstants
{
    std::optional<std::uint32_t> numRaySamples{std::nullopt};
    /// How many random texels each transmittance column spans in the shared noise texture.
    std::uint32_t randTextureBreath;
};

/// @brief Builds the specialization constants for the transmittance precompute pipeline.
star::StarPipelineConstants MakePrecomputeTransmittanceConstants(const TransmittanceConstants &overrides);

struct ColorConstants
{
    std::optional<bool> enableTrilinearInterpolationGridLookups{std::nullopt};
};
/// @brief Function populates the provided CreatePipeline command with the necessary pipeline constants for the color
/// march pass.
/// @param cmd The CreatePipeline command to populate with pipeline constants.
void GetPipelineConstantsForColorMarch(star::command::pipeline::CreatePipeline &cmd,
                                       const ColorConstants &overrideDefaults);
} // namespace render_system::fog