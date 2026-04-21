#include "render_system/fog/commands/FullPass.hpp"

void render_system::fog::commands::FullPass::recordPreCommands(const PassInfo &vInfo, vk::CommandBuffer cmdBuf,
                                                               const star::common::FrameTracker &ft)
{
    if (m_preMemCmds.has_value())
        m_preMemCmds.value().recordPreCommands(vInfo, cmdBuf, ft);
}

void render_system::fog::commands::FullPass::recordPostCommands(const PassInfo &vInfo, vk::CommandBuffer cmdBuf,
                                                                const star::common::FrameTracker &ft)
{
    if (m_postMemCmds.has_value())
        m_postMemCmds.value().recordPostCommands(vInfo, cmdBuf, ft);
}

void render_system::fog::commands::FullPass::recordCommands(const render_system::FogDispatchInfo &dInfo,
                                                            const PassPipelineInfo &passInfo, vk::CommandBuffer cmdBuf,
                                                            const star::common::FrameTracker &ft)
{
    if (m_mainCmds.has_value())
        m_mainCmds.value().recordCommands(dInfo, passInfo, cmdBuf, ft);
}
