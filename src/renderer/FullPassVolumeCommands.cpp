#include "renderer/FullPassVolumeCommands.hpp"

void renderer::FullPassVolumeCommands::recordPreCommands(const VolumePassInfo &vInfo, vk::CommandBuffer cmdBuf,
                                                         const star::common::FrameTracker &ft)
{
    if (m_preMemCmds.has_value())
        m_preMemCmds.value().recordPreCommands(vInfo, cmdBuf, ft);
}

void renderer::FullPassVolumeCommands::recordPostCommands(const VolumePassInfo &vInfo, vk::CommandBuffer cmdBuf,
                                                          const star::common::FrameTracker &ft)
{
    if (m_postMemCmds.has_value())
        m_postMemCmds.value().recordPostCommands(vInfo, cmdBuf, ft);
}

void renderer::FullPassVolumeCommands::recordCommands(vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft)
{
    m_mainCmds.recordCommands(cmdBuf, ft);
}
