#pragma once

#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

class VolumeRenderer;

namespace render_system::fog::commands
{
class Color
{
    VolumeRenderer *m_me{nullptr};

  public:
    explicit Color(VolumeRenderer *me) : m_me(me) {};

    void recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo,
                        vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);
};
} // namespace render_system::fog::commands