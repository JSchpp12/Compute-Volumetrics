#pragma once

#include "render_system/FogDispatchInfo.hpp"
#include "renderer/VolumeColorCommands.hpp"
#include "renderer/VolumeDistanceCommands.hpp"

#include <variant>

namespace renderer
{
class VolumeComputeCommandsContributor
{
    std::variant<VolumeColorCommands, VolumeDistanceCommands> m_approach;

  public:
    explicit VolumeComputeCommandsContributor(VolumeColorCommands approach) : m_approach(std::move(approach))
    {
    }
    explicit VolumeComputeCommandsContributor(VolumeDistanceCommands approach) : m_approach(std::move(approach))
    {
    }
    void recordCommands(const render_system::FogDispatchInfo &dInfo, const renderer::VolumePassPipelineInfo &pipeInfo,
                        vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);
};
} // namespace renderer