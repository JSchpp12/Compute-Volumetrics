#include "renderer/VolumeComputeCommandsContributor.hpp"

void renderer::VolumeComputeCommandsContributor::recordCommands(const render_system::FogDispatchInfo &dInfo,
                                                                const renderer::VolumePassPipelineInfo &pipeInfo,
                                                                vk::CommandBuffer cmdBuffer,
                                                                const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<VolumeColorCommands>(m_approach))
        std::get<VolumeColorCommands>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuffer, ft);

    else if (std::holds_alternative<VolumeDistanceCommands>(m_approach))
        std::get<VolumeDistanceCommands>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuffer, ft);
}