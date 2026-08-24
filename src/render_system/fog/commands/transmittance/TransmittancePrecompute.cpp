#include "render_system/fog/commands/transmittance/TransmittancePrecompute.hpp"

#include <vulkan/vulkan.hpp>

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

void TransmittancePrecompute::recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo,
                                             vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft)
{
    assert(pipeInfo.transmittancePipe.pipeline);

    AddMemoryBarrier(pipeInfo, cmdBuffer);

    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeInfo.transmittancePipe.pipeline);

    // static sets (0,1) + the transmittance per-draw set (2). The transmittance
    // march does not read the depth-test image (set 3) -- rayInit already gated
    // the active rays against the sun depth.
    auto sets = pipeInfo.staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    {
        assert(pipeInfo.transmittanceOnlyShaderInfo != nullptr);

        auto dynamicSets =
            pipeInfo.transmittanceOnlyShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
        sets.insert(sets.end(), dynamicSets.begin(), dynamicSets.end());
    }

    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.transmittancePipe.layout, 0,
                                 static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

    assert(dInfo.indirectBuffer);
    cmdBuffer.dispatchIndirect(dInfo.indirectBuffer, 0);
}
} // namespace render_system::fog::commands