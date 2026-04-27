#include "render_system/fog/commands/ComputeContributor.hpp"

#include "render_system/fog/struct/ShaderPushInfo.hpp"

static void RecPushConsts(vk::CommandBuffer cmdBuf, const render_system::fog::DispatchInfo &dInfo,
                          vk::PipelineLayout layout)
{
    render_system::fog::ShaderPushInfo pushInfo{.stepsPerDispatch = 0};
    cmdBuf.pushConstants(layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushInfo), &pushInfo);
}

void render_system::fog::commands::ComputeContributor::recordCommands(const render_system::fog::DispatchInfo &dInfo,
                                                                      const PassPipelineInfo &pipeInfo,
                                                                      vk::CommandBuffer cmdBuf,
                                                                      const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<Color>(m_approach))
    {
        RecPushConsts(cmdBuf, dInfo, pipeInfo.colorPipe.layout);
        std::get<Color>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);
    }
    else if (std::holds_alternative<Distance>(m_approach))
    {
        RecPushConsts(cmdBuf, dInfo, pipeInfo.distancePipe.layout);
        std::get<Distance>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);
    }
    else if (std::holds_alternative<Init>(m_approach))
    {
        std::get<Init>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);
    }
    else if (std::holds_alternative<IndirectDispatch>(m_approach))
    {
        std::get<IndirectDispatch>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);
    }
}