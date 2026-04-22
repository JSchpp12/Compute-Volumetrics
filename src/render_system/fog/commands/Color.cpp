#include "render_system/fog/commands/Color.hpp"

#include "renderer/VolumeRenderer.hpp"

void render_system::fog::commands::Color::recordCommands(const DispatchInfo &dInfo,
                                                         const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuffer,
                                                         const star::common::FrameTracker &ft)
{
    assert(pipeInfo.colorPipe.pipeline);
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeInfo.colorPipe.pipeline); 

    assert(pipeInfo.staticShaderInfo != nullptr);
    auto sets = pipeInfo.staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    {
        assert(pipeInfo.colorOnlyShaderInfo != nullptr);

        auto dynamicSets = pipeInfo.colorOnlyShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
        sets.insert(sets.end(), dynamicSets.begin(), dynamicSets.end());
    }

    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.colorPipe.layout, 0,
                                 static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

    cmdBuffer.dispatch(dInfo.workgroupSize[0], dInfo.workgroupSize[1], 1);
}