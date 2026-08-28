#include "render_system/fog/commands/Color.hpp"

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

void Color::recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuffer,
                           const star::common::FrameTracker &ft)
{
    assert(pipeInfo.colorPipe.pipeline);

    AddMemoryBarrier(pipeInfo, cmdBuffer);

    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeInfo.colorPipe.pipeline);

    assert(pipeInfo.staticShaderInfo != nullptr);
    assert(pipeInfo.colorOnlyShaderInfo != nullptr);
    auto sets = pipeInfo.colorOnlyShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    const uint32_t firstSet = pipeInfo.colorOnlyShaderInfo->getBaseSet();

    {
        assert(pipeInfo.sceneDepthShaderInfo != nullptr);

        auto depthSets = pipeInfo.sceneDepthShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
        assert(pipeInfo.sceneDepthShaderInfo->getBaseSet() == firstSet + sets.size() &&
               "sceneDepth shader info baseSet does not match concatenation order");
        sets.insert(sets.end(), depthSets.begin(), depthSets.end());
    }

    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.colorPipe.layout, firstSet,
                                 static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

    assert(dInfo.indirectBuffer);
    cmdBuffer.dispatchIndirect(dInfo.indirectBuffer, 0);
}
} // namespace render_system::fog::commands