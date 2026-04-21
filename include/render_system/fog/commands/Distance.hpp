#pragma once

#include "render_system/FogDispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

class VisibilityDistanceCompute;

namespace render_system::fog::commands
{
class Distance
{
    VisibilityDistanceCompute *m_me{nullptr};

  public:
    explicit Distance(VisibilityDistanceCompute *me) : m_me{me}
    {
    }

    void recordCommands(const render_system::FogDispatchInfo &dispatchInfo,
                        const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuf,
                        const star::common::FrameTracker &ft);
};
} // namespace renderer