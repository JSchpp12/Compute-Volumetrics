#pragma once

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace renderer::volume
{
/// One entry in the shared compute layout assembly. The baseSet (which pipeline
/// descriptor-set number this shader info starts at) is no longer carried here
/// -- it is read from the StarShaderInfo itself (set by the DescriptorRecipe via
/// setBaseSet), so there is a single source of truth for the starting set index.
struct ShaderInfoEntry
{
    std::unique_ptr<star::StarShaderInfo> *info{nullptr};
};

/// Descriptor-set layouts the shared compute layout is assembled from. The
/// recipe reads each entry's baseSet from the StarShaderInfo, sorts on it, and
/// asserts contiguity (e.g. static 0, dynamic 2, depth 3).
struct ShaderInfos
{
    ShaderInfoEntry staticInfo{};
    ShaderInfoEntry dynamicInfo{};
    ShaderInfoEntry depthInfo{};
};

/// Builds the one shared vk::PipelineLayout every volume compute pipeline binds
/// against. Constructed once; the resulting layout is handed to each
/// VolumePipelineRecipe.
///
/// `shaderInfos` are the primary contributions whose set layouts are baked into
/// the pipeline layout. `validationInfos` are StarShaderInfo that bind against
/// the same pipeline layout at the same set indices but were built separately
/// (e.g. the transmittance pass binds m_shadowShaderInfo at set 2 and
/// m_shadowDepthShaderInfo at set 3 against a layout authored from
/// dynamicInfo/depthInfo). They are checked isCompatibleWith() against the
/// primary at each overlapping set but are not baked into the layout.
struct ComputePipelineLayoutRecipe
{
    star::core::device::DeviceContext *context{nullptr};
    ShaderInfos shaderInfos{};
    std::vector<ShaderInfoEntry> validationInfos{};
    std::unique_ptr<vk::PipelineLayout> *out{nullptr};

    int operator()();
};
} // namespace renderer::volume