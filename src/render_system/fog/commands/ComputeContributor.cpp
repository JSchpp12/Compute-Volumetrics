#include "render_system/fog/commands/ComputeContributor.hpp"

void render_system::fog::commands::ComputeContributor::recordCommands(const render_system::FogDispatchInfo &dInfo,
                                                                      const PassPipelineInfo &pipeInfo,
                                                                      vk::CommandBuffer cmdBuffer,
                                                                      const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<Color>(m_approach))
        std::get<Color>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuffer, ft);

    else if (std::holds_alternative<Distance>(m_approach))
        std::get<Distance>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuffer, ft);
}