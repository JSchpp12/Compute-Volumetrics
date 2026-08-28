#include "render_system/fog/commands/transmittance/TransmittancePrecompute.hpp"

#include <array>
#include <cassert>
#include <cmath>

namespace render_system::fog::commands
{
TransmittancePrecompute::TransmittancePrecompute(const vk::Extent2D &transmittanceMapResolution)
{
    constexpr uint32_t kWorkgroupSize = 8u;

    m_dispatchX = static_cast<uint32_t>(
        std::ceil(static_cast<float>(transmittanceMapResolution.width) / static_cast<float>(kWorkgroupSize)));
    m_dispatchY = static_cast<uint32_t>(
        std::ceil(static_cast<float>(transmittanceMapResolution.height) / static_cast<float>(kWorkgroupSize)));

    if (m_dispatchX == 0u)
        m_dispatchX = 1u;
    if (m_dispatchY == 0u)
        m_dispatchY = 1u;
}

void TransmittancePrecompute::recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo,
                                             vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft)
{
    (void)dInfo; // no indirect dispatch in the direct 2D march
    assert(pipeInfo.transmittancePipe.pipeline);

    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeInfo.transmittancePipe.pipeline);

    // static sets (0,1) + the transmittance per-draw set (2) + the sun depth
    // depth-test set (3). The march samples the non-compare sun depth at set 3
    // to find each column's ground (surface) depth.
    std::array<vk::DescriptorSet, 2> descriptors{};
    size_t numWritten = 0;
    const auto frameInFlight = ft.getCurrent().getFrameInFlightIndex();

    assert(pipeInfo.staticShaderInfo != nullptr);
    assert(pipeInfo.transmittanceOnlyShaderInfo != nullptr);
    assert(pipeInfo.transmittanceOnlyShaderInfo->getNumDescriptorSets(frameInFlight) <= descriptors.size());
    pipeInfo.transmittanceOnlyShaderInfo->getDescriptors(frameInFlight, descriptors.data(), numWritten);

    const uint32_t firstSet = pipeInfo.transmittanceOnlyShaderInfo->getBaseSet();
    assert(firstSet >= 2 && "transmittance dynamic set should follow the two shared static sets");

    {
        assert(pipeInfo.shadowDepthShaderInfo != nullptr &&
               "The transmittance march requires the sun depth (set 3) to sample each column's ground depth");
        assert(numWritten + pipeInfo.shadowDepthShaderInfo->getNumDescriptorSets(frameInFlight) <= descriptors.size());

        pipeInfo.shadowDepthShaderInfo->getDescriptors(frameInFlight, descriptors.data() + numWritten, numWritten);
        assert(pipeInfo.shadowDepthShaderInfo->getBaseSet() == firstSet + 1 &&
               "shadowDepth shader info baseSet does not match concatenation order");
        assert(numWritten == 2 && "transmittance pass expected one dynamic and one depth descriptor set");
    }

    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.transmittancePipe.layout, firstSet,
                                 static_cast<uint32_t>(numWritten), descriptors.data(), 0, VK_NULL_HANDLE);

    // direct 2D dispatch: one workgroup per 8x8 block of transmittance-map columns
    cmdBuffer.dispatch(m_dispatchX, m_dispatchY, 1);
}
} // namespace render_system::fog::commands