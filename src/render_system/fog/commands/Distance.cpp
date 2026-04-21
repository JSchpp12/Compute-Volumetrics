#include "render_system/fog/commands/Distance.hpp"

#include "VisibilityDistanceCompute.hpp"

void render_system::fog::commands::Distance::recordCommands(const render_system::FogDispatchInfo &dInfo,
                                                            const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuf,
                                                            const star::common::FrameTracker &ft)
{
    // also need to bind the static sets from other set as these are now recorded on a different command buffer

    auto sets = pipeInfo.staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    for (auto &set : m_me->m_dynamicShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex()))
    {
        sets.push_back(set);
    }

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_me->m_marchedPipeline.vkLayout, 0, sets.size(),
                              sets.data(), 0, VK_NULL_HANDLE);

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, m_me->m_marchedPipeline.vkPipeline);
    cmdBuf.dispatch(dInfo.workgroupSize[0], dInfo.workgroupSize[1], 1);
}