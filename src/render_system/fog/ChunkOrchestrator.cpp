#include "render_system/fog/ChunkOrchestrator.hpp"

void render_system::fog::ChunkOrchestrator::cleanupRender(star::core::device::DeviceContext &ctx)
{
    m_cmdBuf.cleanupRender(ctx.getDevice().getVulkanDevice());
}

std::optional<vk::SemaphoreSubmitInfo> render_system::fog::ChunkOrchestrator::getSignalInfo(
    const star::common::FrameTracker &ft) const noexcept
{
    if (m_syncApproach.has_value())
        return m_syncApproach.value().getSignalInfo();

    return std::nullopt;
}

std::optional<render_system::fog::WaitInfo> render_system::fog::ChunkOrchestrator::getWaitInfo(
    const star::common::FrameTracker &ft) noexcept
{
    if (m_syncApproach.has_value())
        return m_syncApproach.value().getWaitInfo();

    return std::nullopt;
}

void render_system::fog::ChunkOrchestrator::recordCommands(const DispatchInfo &dInfo, const PassInfo &vInfo,
                                                           const PassPipelineInfo &pipeInfo,
                                                           const star::common::FrameTracker &ft, Fog::Type type)
{
    const size_t fi = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());

    auto &cmdBuf = m_cmdBuf.buffer(fi);
    m_cmdBuf.begin(fi);

    for (auto &app : m_cmdApproaches)
    {
        // need to wait for last submission to finish before calling begin
        app.recordPreCommands(vInfo, cmdBuf, ft);

        if (m_isReady)
            app.recordCommands(dInfo, pipeInfo, cmdBuf, ft);

        app.recordPostCommands(vInfo, cmdBuf, ft);
    }

    cmdBuf.end();
}