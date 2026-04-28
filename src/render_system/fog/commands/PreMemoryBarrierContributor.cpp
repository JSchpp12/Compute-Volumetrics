#include "render_system/fog/commands/PreMemoryBarrierContributor.hpp"

void render_system::fog::commands::PreMemoryBarrierContributor::recordPreCommands(const PassInfo &tInfo,
                                                                                  vk::CommandBuffer cmdBuf,
                                                                                  const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<color::PreMemoryBarrierRecorder>(m_policy))
    {
        std::get<color::PreMemoryBarrierRecorder>(m_policy).recordCommands(tInfo, ft, cmdBuf);
    }
    else if (std::holds_alternative<distance::PreMemoryBarrierRecorder>(m_policy))
    {
        std::get<distance::PreMemoryBarrierRecorder>(m_policy).recordCommands(tInfo, ft, cmdBuf);
    }
}