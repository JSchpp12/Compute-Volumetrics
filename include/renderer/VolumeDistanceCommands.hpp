#pragma once

#include "render_system/FogDispatchInfo.hpp"
#include "renderer/VolumePassInfo.hpp"

#include <star_common/FrameTracker.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <vulkan/vulkan.hpp>

class VisibilityDistanceCompute;

namespace renderer
{
class VolumeDistanceCommands
{
    VisibilityDistanceCompute *m_me{nullptr};

  public:
    explicit VolumeDistanceCommands(VisibilityDistanceCompute *me) : m_me{me}
    {
    }

    void recordCommands(const render_system::FogDispatchInfo &dispatchInfo,
                        const renderer::VolumePassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuf,
                        const star::common::FrameTracker &ft);
};
} // namespace renderer