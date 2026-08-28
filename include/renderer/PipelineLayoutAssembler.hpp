#pragma once

#include <starlight/core/device/StarDevice.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <vulkan/vulkan.hpp>

#include <vector>

namespace renderer
{
/// One StarShaderInfo's contribution to a shared pipeline layout. The pipeline
/// set this info starts at (baseSet) is read from the StarShaderInfo itself
/// (StarShaderInfo::getBaseSet), so the starting set index has a single source
/// of truth. `primary` contributions are baked into the pipeline layout;
/// `primary == false` contributions are StarShaderInfo that bind against the
/// same pipeline set indices but were built separately -- they are validated for
/// compatibility (StarShaderInfo::isSetLayoutCompatible) against the primary at
/// every overlapping set, but their layouts are NOT baked into the pipeline layout.
struct LayoutContribution
{
    star::StarShaderInfo *info{nullptr};
    bool primary{true};
};

/// Assembles a vk::PipelineLayout from one or more StarShaderInfo contributions.
/// Enforces:
///   * contiguity -- primary contributions must cover pipeline sets 0..N-1 with no
///     gaps and no two primaries may claim the same set index.
///   * compatibility -- every validation-only contribution is checked
///     isSetLayoutCompatible() against the primary layout baked into each set it
///     targets. Catches the case where multiple StarShaderInfo are bound against
///     a single shared pipeline layout at the same set index (e.g. a color pass
///     and a transmittance pass sharing one compute layout).
/// Push-constant ranges are optional and applied verbatim. Used by both the
/// volume compute layout (ComputePipelineLayoutRecipe) and the distance compute
/// layout (CreatePipelines) so they share one assembly path.
struct PipelineLayoutAssembler
{
    star::core::device::StarDevice *device{nullptr};
    std::vector<LayoutContribution> contributions{};
    std::vector<vk::PushConstantRange> pushConstants{};

    /// Build and return the vk::PipelineLayout. Ownership of the returned handle
    /// is the caller's responsibility (destroy via the device when done).
    vk::PipelineLayout operator()() const;
};
} // namespace renderer