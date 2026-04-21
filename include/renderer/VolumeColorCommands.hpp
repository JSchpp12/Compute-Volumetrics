#pragma once

#include "render_system/FogDispatchInfo.hpp"
#include "renderer/VolumePassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <variant>

class VolumeRenderer;

namespace renderer
{
class VolumeColorCommands
{
    VolumeRenderer *m_me{nullptr};

  public:
    explicit VolumeColorCommands(VolumeRenderer *me) : m_me(me){};

    void recordCommands(const render_system::FogDispatchInfo &dInfo, const renderer::VolumePassPipelineInfo &pipeInfo,
                        vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);
};
} // namespace renderer