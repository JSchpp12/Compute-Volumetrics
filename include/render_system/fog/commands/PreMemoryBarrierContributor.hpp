#pragma once

#include "render_system/fog/commands/PreMemoryBarrierDifferentFamilies.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <variant>

namespace render_system::fog::commands
{

class PreMemoryBarrierContributor
{
    std::variant<PreMemoryBarrierDifferentFamilies> m_approach;

  public:
    explicit PreMemoryBarrierContributor(PreMemoryBarrierDifferentFamilies approach) : m_approach(std::move(approach))
    {
    }

    void recordPreCommands(const PassInfo &vInfo, vk::CommandBuffer cmdBuf,
                           const star::common::FrameTracker &ft);
};
} // namespace renderer