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

    size_t numWritten{0};
    // also need to bind the static sets from other set as these are now recorded on a different command buffer
    pipeInfo.staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex(), m_descriptors.data(),
                                              numWritten);

    const uint32_t firstSet = pipeInfo.staticShaderInfo->getBaseSet();
    assert(pipeInfo.distanceOnlyShaderInfo != nullptr);
    assert(pipeInfo.distanceOnlyShaderInfo->getBaseSet() == firstSet + sets.size() &&
           "distanceOnly shader info baseSet does not match concatenation order");

    pipeInfo.distanceOnlyShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex(), &m_descriptors[numWritten],
                                                    numWritten);

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.distancePipe.layout, firstSet, numWritten,
                              m_descriptors.data(), 0, VK_NULL_HANDLE);

    assert(dInfo.indirectBuffer);
    cmdBuf.dispatchIndirect(dInfo.indirectBuffer, 0);
}
} // namespace render_system::fog::commands