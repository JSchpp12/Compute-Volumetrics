#include "render_system/fog/ChunkOrchestrator.hpp"

#include "render_system/fog/struct/ShaderFlags.hpp"
#include "render_system/fog/struct/ShaderPushInfo.hpp"

namespace render_system::fog
{

static void RecPushConsts(vk::CommandBuffer cmdBuf, const render_system::fog::DispatchInfo &dInfo,
                          vk::PipelineLayout layout, uint32_t optionFlags)
{
    render_system::fog::ShaderPushInfo pushInfo{.flags = {std::move(optionFlags)}, .stepsPerDispatch = 0};
    cmdBuf.pushConstants(layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushInfo), &pushInfo);
}

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
                                                           const star::common::FrameTracker &ft)
{
    const size_t fi = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());

    auto &cmdBuf = m_cmdBuf.buffer(fi);
    m_cmdBuf.begin(fi);

    if (m_isReady)
    {
        RecPushConsts(cmdBuf, dInfo, pipeInfo.colorPipe.layout, dInfo.shaderOptionFlags);
    }

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

} // namespace render_system::fog
