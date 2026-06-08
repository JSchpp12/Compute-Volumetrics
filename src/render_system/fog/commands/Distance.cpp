#include "render_system/fog/commands/Distance.hpp"

#include "VisibilityDistanceCompute.hpp"

namespace render_system::fog::commands
{
static void AddMemoryBarrier(const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuf)
{
    vk::BufferMemoryBarrier2 memBarr =
        vk::BufferMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits2::eDrawIndirect | vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eShaderRead)
            .setBuffer(pipeInfo.indirectDispatchBuffer)
            .setOffset(0)
            .setSize(vk::WholeSize);

    cmdBuf.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarrierCount(1).setPBufferMemoryBarriers(&memBarr));
}

void render_system::fog::commands::Distance::recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo,
                                                            vk::CommandBuffer cmdBuf,
                                                            const star::common::FrameTracker &ft)
{
    assert(pipeInfo.distancePipe.pipeline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeInfo.distancePipe.pipeline);

    AddMemoryBarrier(pipeInfo, cmdBuf);
    // also need to bind the static sets from other set as these are now recorded on a different command buffer
    auto sets = pipeInfo.staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    for (auto &set : pipeInfo.distanceOnlyShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex()))
    {
        sets.push_back(set);
    }

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.distancePipe.layout, 0, sets.size(),
                              sets.data(), 0, VK_NULL_HANDLE);

    assert(dInfo.indirectBuffer);
    cmdBuf.dispatchIndirect(dInfo.indirectBuffer, 0);
}
} // namespace render_system::fog::commands