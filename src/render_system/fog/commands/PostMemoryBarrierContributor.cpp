#include "render_system/fog/commands/PostMemoryBarrierContributor.hpp"

void render_system::fog::commands::PostMemoryBarrierContributor::recordPostCommands(
    const PassInfo &vInfo, vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<PostMemoryBarrierDifferentFamilies>(m_approach))
    {
        std::get<PostMemoryBarrierDifferentFamilies>(m_approach).recordPostCommands(vInfo, cmdBuf, ft);
    }
}