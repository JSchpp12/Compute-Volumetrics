#include "render_system/fog/commands/Distance.hpp"

#include "VisibilityDistanceCompute.hpp"

void render_system::fog::commands::Distance::recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo,
                                                            vk::CommandBuffer cmdBuf,
                                                            const star::common::FrameTracker &ft)
{
    assert(pipeInfo.distancePipe.pipeline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeInfo.distancePipe.pipeline);

    // also need to bind the static sets from other set as these are now recorded on a different command buffer
    auto sets = pipeInfo.staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    for (auto &set : pipeInfo.distanceOnlyShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex()))
    {
        sets.push_back(set);
    }

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.distancePipe.layout, 0, sets.size(), sets.data(), 0,
                              VK_NULL_HANDLE);

    assert(dInfo.indirectBuffer); 
    cmdBuf.dispatchIndirect(dInfo.indirectBuffer, 0);
}