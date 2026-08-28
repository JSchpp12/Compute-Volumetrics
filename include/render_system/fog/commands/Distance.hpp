#pragma once

#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <array>

namespace render_system::fog::commands
{
class Distance
{
  public:
    void recordCommands(const DispatchInfo &dispatchInfo, const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuf,
                        const star::common::FrameTracker &ft);

  private:
    std::array<vk::DescriptorSet, 3> m_descriptors;
};
} // namespace render_system::fog::commands