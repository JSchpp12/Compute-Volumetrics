#include "render_system/fog/commands/PreMemoryBarrierContributor.hpp"

void render_system::fog::commands::PreMemoryBarrierContributor::recordPreCommands(const PassInfo &tInfo,
                                                                                  vk::CommandBuffer cmdBuf,
                                                                                  const star::common::FrameTracker &ft)
{
    for (const auto &policy : m_policies)
    {
        if (std::holds_alternative<color::FrameInputPreMemoryBarrierRecorder>(policy))
        {
            std::get<color::FrameInputPreMemoryBarrierRecorder>(policy).recordCommands(tInfo, ft, cmdBuf);
        }
        else if (std::holds_alternative<color::PreMemoryBarrierRecorder>(policy))
        {
            std::get<color::PreMemoryBarrierRecorder>(policy).recordCommands(tInfo, ft, cmdBuf);
        }
        else if (std::holds_alternative<distance::PreMemoryBarrierRecorder>(policy))
        {
            std::get<distance::PreMemoryBarrierRecorder>(policy).recordCommands(tInfo, ft, cmdBuf);
        }
        else if (std::holds_alternative<transmittance::PreMemoryBarrierRecorder>(policy))
        {
            std::get<transmittance::PreMemoryBarrierRecorder>(policy).recordCommands(tInfo, ft, cmdBuf);
        }
    }
}
