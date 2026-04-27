#include "render_system/fog/commands/PostMemoryBarrierContributor.hpp"

void render_system::fog::commands::PostMemoryBarrierContributor::recordPostCommands(
    const PassInfo &vInfo, vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<color::PostMemoryBarrierRecorder>(m_approach))
    {
        std::get<color::PostMemoryBarrierRecorder>(m_approach).recordCommands(vInfo, ft, cmdBuf);
    }
    else if (std::holds_alternative<distance::PostMemoryBarrierRecorder>(m_approach))
    {

    }
}