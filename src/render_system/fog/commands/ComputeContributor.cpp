#include "render_system/fog/commands/ComputeContributor.hpp"

void render_system::fog::commands::ComputeContributor::recordCommands(const render_system::fog::DispatchInfo &dInfo,
                                                                      const PassPipelineInfo &pipeInfo,
                                                                      vk::CommandBuffer cmdBuf,
                                                                      const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<Color>(m_approach))
    {
        std::get<Color>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);
    }
    else if (std::holds_alternative<Distance>(m_approach))
    {
        std::get<Distance>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);
    }
    else if (std::holds_alternative<Init>(m_approach))
    {
        std::get<Init>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);
    }
    else if (std::holds_alternative<IndirectDispatch>(m_approach))
    {
        std::get<IndirectDispatch>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);
    }
}