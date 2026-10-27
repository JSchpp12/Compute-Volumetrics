#pragma once

#include <star_common/Handle.hpp>
#include <starlight/command/pipeline/CreatePipeline.hpp>
#include <starlight/core/device/DeviceContext.hpp>

#include <functional>

#include <vulkan/vulkan.hpp>

namespace renderer::volume
{

/// Instrtuctions on how to build one volume compute pipeline with a shared layout.
class VolumePipelineRecipe
{
  public:
    star::core::device::DeviceContext *context{nullptr};
    /// Already-created compute shader this pipeline is built from.
    star::Handle shader{};
    // make sure to set before operator()
    const vk::PipelineLayout *layout{nullptr};
    star::Handle *outHandle{nullptr};
    std::function<void(star::command::pipeline::CreatePipeline &)> pipelineConstantsProvider = nullptr;

    int operator()();

  private:
    star::Handle buildPipeline(star::core::device::DeviceContext *context) const;
};
} // namespace renderer::volume