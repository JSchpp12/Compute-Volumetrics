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

void renderer::FullPassVolumeCommands::recordCommands(const render_system::FogDispatchInfo &dInfo,
                                                      const renderer::VolumePassPipelineInfo &passInfo,
                                                      vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft)
{
    if (m_mainCmds.has_value())
        m_mainCmds.value().recordCommands(dInfo, passInfo, cmdBuf, ft);
}
