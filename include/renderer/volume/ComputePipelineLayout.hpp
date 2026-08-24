#pragma once

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>

namespace renderer::volume
{
/// Descriptor-set layouts the shared compute layout is assembled from.
/// Order when bound: staticInfo (sets 0-1), dynamicInfo (set 2), depthInfo (set 3).
struct ShaderInfos
{
    std::unique_ptr<star::StarShaderInfo> *staticInfo{nullptr};
    std::unique_ptr<star::StarShaderInfo> *dynamicInfo{nullptr};
    std::unique_ptr<star::StarShaderInfo> *depthInfo{nullptr};
};

/// Builds the one shared vk::PipelineLayout every volume compute pipeline
/// binds against. Constructed once; the resulting layout is handed to each
/// VolumePipelineRecipe.
struct ComputePipelineLayoutRecipe
{
    star::core::device::DeviceContext *context{nullptr};
    ShaderInfos shaderInfos{};
    std::unique_ptr<vk::PipelineLayout> *out{nullptr};

    int operator()();
};
} // namespace renderer::volume