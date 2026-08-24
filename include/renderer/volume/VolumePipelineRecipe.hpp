#pragma once

#include <starlight/core/device/DeviceContext.hpp>

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <string>

namespace renderer::volume
{
/// Instrtuctions on how to build one volume compute pipeline with a shared layout.
struct VolumePipelineRecipe
{
    star::core::device::DeviceContext *context{nullptr};
    std::string shaderFile{};
    // make sure to set before operator()
    const vk::PipelineLayout *layout{nullptr};
    star::Handle *outHandle{nullptr};

    /// Optional. When set, a one-shot waiter copies the resolved vk::Pipeline into this cached pointer (used by init /
    /// dispatchCmd today).
    vk::Pipeline *outCachedPipeline{nullptr};

    int operator()();
};
} // namespace renderer::volume