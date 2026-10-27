#pragma once

#include <star_common/Handle.hpp>

#include <optional>

namespace star::command::pipeline
{
class CreatePipeline;
}

namespace render_system::fog
{
struct TransmittanceConstants
{
    std::optional<std::uint32_t> sizeLocalX{std::nullopt};
    std::optional<std::uint32_t> sizeLocalY{std::nullopt};
    std::optional<std::uint32_t> sizeLocalZ{std::nullopt};
    std::optional<std::uint32_t> numRaySamples{std::nullopt};
};

/// @brief Function populates the provided CreatePipeline command with the necessary pipeline constants for precomputing
/// transmittance.
/// @param cmd The CreatePipeline command to populate with pipeline constants.
void GetPipelineConstantsForPrecomputeTransmittance(star::command::pipeline::CreatePipeline &cmd,
                                                    const TransmittanceConstants &overrideDefaults);

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