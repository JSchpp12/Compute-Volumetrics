#pragma once

#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog::commands
{
class TransmittancePrecompute
{
  public:
    explicit TransmittancePrecompute(const vk::Extent2D &transmittanceMapResolution);

    void recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuffer,
                        const star::common::FrameTracker &ft);

  private:
    uint32_t m_dispatchX{1};
    uint32_t m_dispatchY{1};
};
} // namespace render_system::fog::commands