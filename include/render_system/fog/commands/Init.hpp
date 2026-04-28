#pragma once

#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog::commands
{
class Init
{
    std::array<uint32_t, 2> m_workgroupSize{0, 0};

  public:
    Init(const vk::Extent2D &screenResolution); 

    void recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuffer,
                        const star::common::FrameTracker &ft);
};
}