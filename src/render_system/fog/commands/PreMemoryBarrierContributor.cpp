#include "render_system/fog/commands/PreMemoryBarrierContributor.hpp"

void render_system::fog::commands::PreMemoryBarrierContributor::recordPreCommands(const PassInfo &tInfo,
                                                                                  vk::CommandBuffer cmdBuf,
                                                                                  const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<PreMemoryBarrierDifferentFamilies>(m_approach))
    {
        std::get<PreMemoryBarrierDifferentFamilies>(m_approach).recordPreCommands(tInfo, cmdBuf, ft);
    }
}