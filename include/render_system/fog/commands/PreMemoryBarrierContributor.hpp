#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/color/FrameInputPreMemoryBarrierRecorder.hpp"
#include "render_system/fog/commands/color/PreMemoryBarrierRecorder.hpp"
#include "render_system/fog/commands/distance/PreMemoryBarrierRecorder.hpp"
#include "render_system/fog/commands/transmittance/PreMemoryBarrierRecorder.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <variant>
#include <vector>

namespace render_system::fog::commands
{
using PreRecorderType = std::variant<color::FrameInputPreMemoryBarrierRecorder, color::PreMemoryBarrierRecorder,
                                     distance::PreMemoryBarrierRecorder, transmittance::PreMemoryBarrierRecorder>;

class PreMemoryBarrierContributor
{
    std::vector<PreRecorderType> m_policies;

  public:
    explicit PreMemoryBarrierContributor(PreRecorderType policy) : m_policies{std::move(policy)}
    {
    }

    PreMemoryBarrierContributor(std::initializer_list<PreRecorderType> policies) : m_policies(policies)
    {
    }

    void recordPreCommands(const PassInfo &vInfo, vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);
};
} // namespace render_system::fog::commands
