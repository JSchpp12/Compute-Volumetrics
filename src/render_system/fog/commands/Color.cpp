#include "render_system/fog/commands/Color.hpp"

#include "renderer/VolumeRenderer.hpp"

void render_system::fog::commands::Color::recordCommands(const DispatchInfo &dInfo,
                                                         const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuffer,
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

    cmdBuffer.dispatch(dInfo.workgroupSize[0], dInfo.workgroupSize[1], 1);
}