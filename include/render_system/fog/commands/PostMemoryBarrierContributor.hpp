#pragma once

#include "render_system/fog/commands/color/PostMemoryBarrierRecorder.hpp"
#include "render_system/fog/commands/distance/PostMemoryBarrierRecorder.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <variant>

namespace render_system::fog::commands
{

using PostRecorderType = std::variant<color::PostMemoryBarrierRecorder, distance::PostMemoryBarrierRecorder>;

class PostMemoryBarrierContributor
{
    PostRecorderType m_approach;

  public:
    explicit PostMemoryBarrierContributor(PostRecorderType approach) : m_approach(std::move(approach))
    {
    }

    void recordPostCommands(const PassInfo &vInfo, vk::CommandBuffer cmdBuf,
                            const star::common::FrameTracker &ft);
};
} // namespace renderer