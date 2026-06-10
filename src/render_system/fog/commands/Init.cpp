#include "render_system/fog/commands/Init.hpp"

#include "render_system/fog/struct/DispatchIndirectCommand.hpp"
#include <algorithm>

namespace render_system::fog::commands
{
static void AddBarrierDepCompute(const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuf) noexcept
{
    const auto barrier = vk::BufferMemoryBarrier2()
                             .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                             .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                             .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                             .setDstAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
                             .setBuffer(pipeInfo.indirectDispatchBuffer)
                             .setOffset(0)
                             .setSize(vk::WholeSize);

    cmdBuf.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarrierCount(1).setPBufferMemoryBarriers(&barrier));
}

static void AddBarrierDepPreviousIndirectRead(const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuf,
                                              bool addProtectPreviousWriteToIndirectBarrier) noexcept
{
    vk::PipelineStageFlags2 srcStage = vk::PipelineStageFlagBits2::eDrawIndirect;
    vk::AccessFlags2 srcAccess = vk::AccessFlagBits2::eNone;

    if (addProtectPreviousWriteToIndirectBarrier)
    {
        srcStage |= vk::PipelineStageFlagBits2::eComputeShader;
        srcAccess |= vk::AccessFlagBits2::eShaderStorageWrite;
    }

    const auto barrier = vk::MemoryBarrier2()
                             .setSrcStageMask(srcStage)
                             .setSrcAccessMask(srcAccess)
                             .setDstStageMask(vk::PipelineStageFlagBits2::eClear)
                             .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite);

    cmdBuf.pipelineBarrier2(vk::DependencyInfo().setMemoryBarrierCount(1).setPMemoryBarriers(&barrier));
}

static std::array<uint32_t, 2> CalculateWorkgroupSize(const vk::Extent2D &screenResolution)
{
    const uint32_t width = static_cast<uint32_t>(std::ceil(static_cast<float>(screenResolution.width) / 8.0f));
    const uint32_t height = static_cast<uint32_t>(std::ceil(static_cast<float>(screenResolution.height) / 8.0f));

    return {width, height};
}

Init::Init(const vk::Extent2D &screenResolution, bool needsMemoryBarrierProtectFromPreviousDispatch)
    : m_workgroupSize(CalculateWorkgroupSize(screenResolution)),
      m_needsMemoryBarrierProtectFromPreviousDispatch(needsMemoryBarrierProtectFromPreviousDispatch)
{
}

void Init::recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuffer,
                          const star::common::FrameTracker &ft, std::span<OptionalClearBuffer> optionalClears)
{
    assert(pipeInfo.initPipeline && "Init pipeline should have been provided in PassPipelineInfo");
    assert(pipeInfo.colorPipe.layout != VK_NULL_HANDLE);

    AddBarrierDepPreviousIndirectRead(pipeInfo, cmdBuffer, m_needsMemoryBarrierProtectFromPreviousDispatch);

    cmdBuffer.fillBuffer(pipeInfo.indirectDispatchBuffer, 0, sizeof(DispatchIndirectCommand), 0);
    cmdBuffer.fillBuffer(pipeInfo.indirectDispatchBuffer, sizeof(DispatchIndirectCommand), sizeof(uint32_t), 0);

    for (uint32_t i{ 0 }; i < m_additionalClearCount; i++)
    {
        cmdBuffer.fillBuffer(m_additionalClears[i].buffer, 0, vk::WholeSize, m_additionalClears[i].fillValue);
    }

    AddBarrierDepCompute(pipeInfo, cmdBuffer);

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

    m_additionalClearCount = 0;
}

void Init::setAdditionalClears(std::span<const OptionalClearBuffer> clears)
{
    assert(clears.size() <= MaxAdditionalClears);

    m_additionalClearCount = static_cast<uint32_t>(clears.size());
    std::copy(clears.begin(), clears.end(), m_additionalClears.begin());
}
} // namespace render_system::fog::commands