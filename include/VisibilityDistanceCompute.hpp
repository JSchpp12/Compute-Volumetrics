#pragma once

#include "FogType.hpp"
#include "renderer/volume/ContainerRenderResourceData.hpp"

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <star_common/Handle.hpp>

#include <vulkan/vulkan_core.h>

#include <memory>

class VisibilityDistanceCompute
{
  public:
    void cleanupRender(star::core::device::DeviceContext &context);

    void prepRender(star::core::device::DeviceContext &context, renderer::volume::ContainerRenderResourceData data,
                    std::unique_ptr<star::StarShaderInfo> *sharedComputePipelineLayout);

    void recordCommandBuffer(vk::CommandBuffer commandBuffer, const star::common::FrameTracker &frameTracker,
                             const glm::uvec2 &workgroupSize, Fog::Type type);

    void frameUpdate(star::core::device::DeviceContext &context);

    bool isReady(const star::core::device::DeviceContext &context);

    vk::Pipeline getPipeline() const
    {
        return m_marchedPipeline.vkPipeline;
    }
    vk::PipelineLayout getLayout() const
    {
        return m_marchedPipeline.vkLayout;
    }

    star::StarShaderInfo *getDynamicShaderInfo()
    {
        return m_dynamicShaderInfo.get();
    }

  private:
    struct PipelineData
    {
        star::Handle handle;
        vk::Pipeline vkPipeline{VK_NULL_HANDLE};
        vk::PipelineLayout vkLayout{VK_NULL_HANDLE};
    };
    PipelineData m_marchedPipeline{};
    std::unique_ptr<star::StarShaderInfo> m_dynamicShaderInfo{nullptr};
    bool m_isReady{false};

    void createBuildPipelineWaiter(star::core::device::DeviceContext &context,
                                   renderer::volume::ContainerRenderResourceData data,
                                   std::unique_ptr<star::StarShaderInfo> *sharedComputePipelineLayout);

    void updateRenderingContext(const star::core::device::DeviceContext &context);
};