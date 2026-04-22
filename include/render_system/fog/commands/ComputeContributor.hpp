#pragma once

#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/commands/Color.hpp"
#include "render_system/fog/commands/Distance.hpp"

#include <variant>

namespace render_system::fog::commands
{
class ComputeContributor
{
    std::variant<Color, Distance> m_approach;

  public:
    explicit ComputeContributor(Color approach) : m_approach(std::move(approach))
    {
    }
    explicit ComputeContributor(Distance approach) : m_approach(std::move(approach))
    {
    }
    void recordCommands(const render_system::fog::DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo,
                        vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);
};
} // namespace render_system::fog::commands