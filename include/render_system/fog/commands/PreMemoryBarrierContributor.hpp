#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/color/PreMemoryBarrierRecorder.hpp"
#include "render_system/fog/commands/distance/PreMemoryBarrierRecorder.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <variant>

namespace render_system::fog::commands
{
using PreRecorderType = std::variant<color::PreMemoryBarrierRecorder, distance::PreMemoryBarrierRecorder>;

class PreMemoryBarrierContributor
{
    PreRecorderType m_policy;

  public:
    explicit PreMemoryBarrierContributor(PreRecorderType policy) : m_policy(std::move(policy))
    {
    }

    void recordPreCommands(const PassInfo &vInfo, vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);
};
} // namespace render_system::fog::commands