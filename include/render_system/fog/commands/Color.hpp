#pragma once

#include "render_system/FogDispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <variant>

class VolumeRenderer;

namespace render_system::fog::commands
{
class Color
{
    VolumeRenderer *m_me{nullptr};

  public:
    explicit Color(VolumeRenderer *me) : m_me(me) {};

    void recordCommands(const render_system::FogDispatchInfo &dInfo, const PassPipelineInfo &pipeInfo,
                        vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);
};
} // namespace render_system::fog::commands