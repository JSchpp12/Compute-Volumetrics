#include "render_system/fog/commands/ComputeContributor.hpp"

#include "render_system/fog/ShaderPushInfo.hpp"

void render_system::fog::commands::ComputeContributor::recordCommands(const render_system::fog::DispatchInfo &dInfo,
                                                                      const PassPipelineInfo &pipeInfo,
                                                                      vk::CommandBuffer cmdBuf,
                                                                      const star::common::FrameTracker &ft)
{
    {
        ShaderPushInfo pushInfo{.dispatchOffsetPixels = {dInfo.chunkOffsetPixels[0], dInfo.chunkOffsetPixels[1]}};
        cmdBuf.pushConstants(pipeInfo.pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushInfo),
                             &pushInfo);
    }

    if (std::holds_alternative<Color>(m_approach))
        std::get<Color>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);

    else if (std::holds_alternative<Distance>(m_approach))
        std::get<Distance>(m_approach).recordCommands(dInfo, pipeInfo, cmdBuf, ft);
}