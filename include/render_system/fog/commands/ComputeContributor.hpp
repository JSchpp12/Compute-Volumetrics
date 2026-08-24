#pragma once

#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/commands/Color.hpp"
#include "render_system/fog/commands/Distance.hpp"
#include "render_system/fog/commands/IndirectDispatch.hpp"
#include "render_system/fog/commands/Init.hpp"
#include "render_system/fog/commands/transmittance/TransmittancePrecompute.hpp"

#include <variant>

namespace render_system::fog::commands
{
class ComputeContributor
{
    std::variant<Color, Distance, Init, IndirectDispatch, TransmittancePrecompute> m_approach;

  public:
    explicit ComputeContributor(Color approach) : m_approach(std::move(approach))
    {
    }
    explicit ComputeContributor(Distance approach) : m_approach(std::move(approach))
    {
    }
    explicit ComputeContributor(Init approach) : m_approach(std::move(approach))
    {
    }
    explicit ComputeContributor(TransmittancePrecompute approach) : m_approach(std::move(approach))
    {
    }
    explicit ComputeContributor(IndirectDispatch approach) : m_approach(std::move(approach))
    {
    }

    void recordCommands(const render_system::fog::DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo,
                        vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);

    void setAdditionalClears(std::span<const Init::OptionalClearBuffer> clears); 

};
} // namespace render_system::fog::commands
