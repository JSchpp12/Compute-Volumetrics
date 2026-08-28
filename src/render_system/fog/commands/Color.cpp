#include "render_system/fog/commands/Color.hpp"

#include <array>
#include <cassert>

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

    std::array<vk::DescriptorSet, 2> descriptors{};
    size_t numWritten = 0;
    const auto frameInFlight = ft.getCurrent().getFrameInFlightIndex();

    assert(pipeInfo.staticShaderInfo != nullptr);
    assert(pipeInfo.colorOnlyShaderInfo != nullptr);
    assert(pipeInfo.colorOnlyShaderInfo->getNumDescriptorSets(frameInFlight) <= descriptors.size());
    pipeInfo.colorOnlyShaderInfo->getDescriptors(frameInFlight, descriptors.data(), numWritten);
    const uint32_t firstSet = pipeInfo.colorOnlyShaderInfo->getBaseSet();

    {
        assert(pipeInfo.sceneDepthShaderInfo != nullptr);
        assert(numWritten + pipeInfo.sceneDepthShaderInfo->getNumDescriptorSets(frameInFlight) <= descriptors.size());

        pipeInfo.sceneDepthShaderInfo->getDescriptors(frameInFlight, descriptors.data() + numWritten, numWritten);
        assert(pipeInfo.sceneDepthShaderInfo->getBaseSet() == firstSet + 1 &&
               "sceneDepth shader info baseSet does not match concatenation order");
        assert(numWritten == 2 && "color pass expected one dynamic and one depth descriptor set");
    }

    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.colorPipe.layout, firstSet,
                                 static_cast<uint32_t>(numWritten), descriptors.data(), 0, VK_NULL_HANDLE);

    assert(dInfo.indirectBuffer);
    cmdBuffer.dispatchIndirect(dInfo.indirectBuffer, 0);
}
} // namespace render_system::fog::commands