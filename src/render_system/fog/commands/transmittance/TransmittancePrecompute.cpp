#include "render_system/fog/commands/transmittance/TransmittancePrecompute.hpp"

#include <vulkan/vulkan.hpp>

#include <cassert>
#include <cmath>

namespace render_system::fog::commands
{
TransmittancePrecompute::TransmittancePrecompute(const vk::Extent2D &transmittanceMapResolution)
{
    constexpr uint32_t kWorkgroupSize = 8u;

    m_dispatchX =
        static_cast<uint32_t>(std::ceil(static_cast<float>(transmittanceMapResolution.width) /
                                        static_cast<float>(kWorkgroupSize)));
    m_dispatchY =
        static_cast<uint32_t>(std::ceil(static_cast<float>(transmittanceMapResolution.height) /
                                        static_cast<float>(kWorkgroupSize)));

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
    auto sets = pipeInfo.staticShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
    const uint32_t firstSet = pipeInfo.staticShaderInfo->getBaseSet();
    {
        assert(pipeInfo.transmittanceOnlyShaderInfo != nullptr);

        auto dynamicSets =
            pipeInfo.transmittanceOnlyShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
        assert(pipeInfo.transmittanceOnlyShaderInfo->getBaseSet() == firstSet + sets.size() &&
               "transmittanceOnly shader info baseSet does not match concatenation order");
        sets.insert(sets.end(), dynamicSets.begin(), dynamicSets.end());
    }
    {
        assert(pipeInfo.shadowDepthShaderInfo != nullptr &&
               "The transmittance march requires the sun depth (set 3) to sample each column's ground depth");

        auto depthSets = pipeInfo.shadowDepthShaderInfo->getDescriptors(ft.getCurrent().getFrameInFlightIndex());
        assert(pipeInfo.shadowDepthShaderInfo->getBaseSet() == firstSet + sets.size() &&
               "shadowDepth shader info baseSet does not match concatenation order");
        sets.insert(sets.end(), depthSets.begin(), depthSets.end());
    }

    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeInfo.transmittancePipe.layout, firstSet,
                                 static_cast<uint32_t>(sets.size()), sets.data(), 0, VK_NULL_HANDLE);

    // direct 2D dispatch: one workgroup per 8x8 block of transmittance-map columns
    cmdBuffer.dispatch(m_dispatchX, m_dispatchY, 1);
}
} // namespace render_system::fog::commands