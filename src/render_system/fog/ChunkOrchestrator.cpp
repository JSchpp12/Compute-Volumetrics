#include "render_system/fog/ChunkOrchestrator.hpp"

void render_system::fog::ChunkOrchestrator::cleanupRender(star::core::device::DeviceContext &ctx)
{
    m_cmdBuf.cleanupRender(ctx.getDevice().getVulkanDevice());
}

vk::SemaphoreSubmitInfo render_system::fog::ChunkOrchestrator::getSignalInfo(const star::common::FrameTracker &ft) const
{
    return m_syncApproach.getSignalInfo();
}

render_system::fog::sync::WaitInfo render_system::fog::ChunkOrchestrator::getWaitInfo(
    const star::common::FrameTracker &ft)
{
    return m_syncApproach.getWaitInfo();
}

void render_system::fog::ChunkOrchestrator::recordCommands(const DispatchInfo &dInfo, const PassInfo &vInfo,
                                                           const PassPipelineInfo &pipeInfo,
                                                           const star::common::FrameTracker &ft, Fog::Type type)
{
    const size_t fi = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
    assert(m_cmdBuf.buffer(ft.getCurrent().getFrameInFlightIndex()));

    // need to wait for last submission to finish before calling begin

    m_cmdBuf.begin(fi);
    m_cmdApproach.recordPreCommands(vInfo, m_cmdBuf.buffer(fi), ft);

    if (m_isReady)
        m_cmdApproach.recordCommands(dInfo, pipeInfo, m_cmdBuf.buffer(fi), ft);

    m_cmdApproach.recordPostCommands(vInfo, m_cmdBuf.buffer(fi), ft);
    m_cmdBuf.buffer(fi).end();
}

vk::CommandBufferSubmitInfo render_system::fog::ChunkOrchestrator::getSubmitInfo(const star::common::FrameTracker &ft)
{
    return vk::CommandBufferSubmitInfo().setCommandBuffer(m_cmdBuf.buffer(ft.getCurrent().getFrameInFlightIndex()));
}
