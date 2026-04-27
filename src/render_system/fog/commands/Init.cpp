#include "render_system/fog/commands/Init.hpp"

#include "render_system/fog/struct/DispatchIndirectCommand.hpp"

namespace render_system::fog::commands
{
static void AddMemoryBarrier(const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuf) noexcept
{
    const auto waitForZero = vk::BufferMemoryBarrier2()
                                 .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                                 .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                                 .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                                 .setDstAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
                                 .setBuffer(pipeInfo.indirectDispatchBuffer)
                                 .setOffset(0)
                                 .setSize(vk::WholeSize);

    cmdBuf.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarrierCount(1).setPBufferMemoryBarriers(&waitForZero));
}

static std::array<uint32_t, 2> CalculateWorkgroupSize(const vk::Extent2D &screenResolution)
{
    const uint32_t width = static_cast<uint32_t>(std::ceil(static_cast<float>(screenResolution.width) / 8.0f));
    const uint32_t height = static_cast<uint32_t>(std::ceil(static_cast<float>(screenResolution.height) / 8.0f));

    return {width, height};
}

Init::Init(const vk::Extent2D &screenResolution) : m_workgroupSize(CalculateWorkgroupSize(screenResolution))
{
}

void Init::recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuffer,
                          const star::common::FrameTracker &ft)
{
    assert(pipeInfo.initPipeline && "Init pipeline should have been provided in PassPipelineInfo");
    assert(pipeInfo.colorPipe.layout != VK_NULL_HANDLE);

    cmdBuffer.fillBuffer(pipeInfo.indirectDispatchBuffer, 0, sizeof(DispatchIndirectCommand), 0);
    cmdBuffer.fillBuffer(pipeInfo.indirectDispatchBuffer, sizeof(DispatchIndirectCommand), sizeof(uint32_t), 0);

    AddMemoryBarrier(pipeInfo, cmdBuffer);

    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeInfo.initPipeline);

    auto sets = pipeInfo.staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    {
        assert(pipeInfo.colorOnlyShaderInfo != nullptr);

        auto dynamicSets = pipeInfo.colorOnlyShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
        sets.insert(sets.end(), dynamicSets.begin(), dynamicSets.end());
    }

    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.colorPipe.layout, 0,
                                 static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

    cmdBuffer.dispatch(m_workgroupSize[0], m_workgroupSize[1], 1);
}
} // namespace render_system::fog::commands