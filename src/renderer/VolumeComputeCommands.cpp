#include "renderer/VolumeComputeCommands.hpp"

#include "renderer/VolumeRenderer.hpp"
#include "render_system/FogShaderPushInfo.hpp"

void renderer::VolumeComputeCommands::recordCommands(vk::CommandBuffer cmdBuffer,
                                                       const star::common::FrameTracker &ft)
{
    m_me->m_renderingContext.pipeline->bind(cmdBuffer);

    auto sets = m_me->m_staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    {
        auto dynamicSets = m_me->m_dynamicShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
        sets.insert(sets.end(), dynamicSets.begin(), dynamicSets.end());
    }

    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *m_me->computePipelineLayout, 0,
                                     static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

    {
        render_system::FogShaderPushInfo pushInfo{};

        cmdBuffer.pushConstants(*m_me->computePipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushInfo),
                                    &pushInfo);
    }

    cmdBuffer.dispatch(m_me->workgroupSize.x, m_me->workgroupSize.y, 1);
}

// vk::SubmitInfo renderer::VolumeComputeCommands::getSubmitInfo()
// {
//     return vk::SubmitInfo();
// }