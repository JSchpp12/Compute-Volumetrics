#include "render_system/fog/commands/IndirectDispatch.hpp"

namespace render_system::fog::commands
{

static void AddMemoryBarrier(const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuf)
{
    const auto dispatchMemBar =
        vk::BufferMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
            .setBuffer(pipeInfo.indirectDispatchBuffer)
            .setOffset(0)
            .setSize(vk::WholeSize);

    cmdBuf.pipelineBarrier2(
        vk::DependencyInfo().setBufferMemoryBarrierCount(1).setPBufferMemoryBarriers(&dispatchMemBar));
}

void IndirectDispatch::recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo,
                                      vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft)
{
    assert(pipeInfo.indirectDispatchPipeline);

    AddMemoryBarrier(pipeInfo, cmdBuf);

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeInfo.indirectDispatchPipeline);

    // todo: move this up to callee/structs as this is used across multiples and getting it multiple times is strange
    auto sets = pipeInfo.staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.colorPipe.layout, 0,
                              static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

    cmdBuf.dispatch(1, 1, 1);
}
} // namespace render_system::fog::commands
