#include "renderer/FullPassVolumeCommands.hpp"

void renderer::FullPassVolumeCommands::recordPreCommands(vk::CommandBuffer cmdBuffer,
                                                         const star::common::FrameTracker &ft)
{
}

void renderer::FullPassVolumeCommands::recordPostCommands(vk::CommandBuffer cmdBuffer,
                                                          const star::common::FrameTracker &ft)
{
}

void renderer::FullPassVolumeCommands::recordCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft)
{
    m_mainCmds.recordCommands(cmdBuffer, ft); 
}
