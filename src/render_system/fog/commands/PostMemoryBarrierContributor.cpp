#include "render_system/fog/commands/PostMemoryBarrierContributor.hpp"

void render_system::fog::commands::PostMemoryBarrierContributor::recordPostCommands(
    const PassInfo &vInfo, vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<color::PostMemoryBarrierRecorder>(m_policy))
    {
        std::get<color::PostMemoryBarrierRecorder>(m_policy).recordCommands(vInfo, ft, cmdBuf);
    }
    else if (std::holds_alternative<distance::PostMemoryBarrierRecorder>(m_policy))
    {
        std::get<distance::PostMemoryBarrierRecorder>(m_policy).recordCommands(vInfo, ft, cmdBuf);
    }
}