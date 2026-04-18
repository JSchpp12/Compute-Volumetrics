#include "renderer/FullPassVolumeCommands.hpp"

void renderer::FullPassVolumeCommands::recordPreCommands(vk::CommandBuffer cmdBuffer,
                                                         const star::common::FrameTracker &ft)
{
    if (m_preMemCmds.has_value())
        m_preMemCmds.value().recordPreCommands(cmdBuffer, ft);
}

void renderer::FullPassVolumeCommands::recordPostCommands(vk::CommandBuffer cmdBuffer,
                                                          const star::common::FrameTracker &ft)
{
    if (m_postMemCmds.has_value())
        m_postMemCmds.value().recordPostCommands(cmdBuffer, ft);
}

void renderer::FullPassVolumeCommands::recordCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft)
{
    m_mainCmds.recordCommands(cmdBuffer, ft);
}
