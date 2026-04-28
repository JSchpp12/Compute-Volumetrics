#pragma once

#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/commands/PostMemoryBarrierContributor.hpp"
#include "render_system/fog/commands/PreMemoryBarrierContributor.hpp"
#include "render_system/fog/commands/ComputeContributor.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <optional>

class VolumeRenderer;

namespace render_system::fog::commands
{

class Pass
{
    std::optional<PreMemoryBarrierContributor> m_preMemCmds{std::nullopt};
    std::optional<PostMemoryBarrierContributor> m_postMemCmds{std::nullopt};
    std::optional<ComputeContributor> m_mainCmds{std::nullopt};

  public:
    Pass() = default;
    Pass(ComputeContributor mainCmds)
        : m_preMemCmds{std::nullopt}, m_postMemCmds{std::nullopt}, m_mainCmds(std::move(mainCmds))
    {
    }
    Pass(ComputeContributor mainCmds, PreMemoryBarrierContributor preMemCmds)
        : m_preMemCmds(std::move(preMemCmds)), m_postMemCmds{std::nullopt}, m_mainCmds(std::move(mainCmds))
    {
    }
    Pass(ComputeContributor mainCmds, PostMemoryBarrierContributor postMemCmds)
        : m_preMemCmds{std::nullopt}, m_postMemCmds(std::move(postMemCmds)), m_mainCmds(std::move(mainCmds))
    {
    }
    Pass(ComputeContributor mainCmds, PreMemoryBarrierContributor preMemCmds,
                           PostMemoryBarrierContributor postMemCmds)
        : m_preMemCmds(std::move(preMemCmds)), m_postMemCmds(std::move(postMemCmds)), m_mainCmds(std::move(mainCmds))
    {
    }

    void recordPreCommands(const PassInfo &tInfo, vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);

    void recordPostCommands(const PassInfo &tInfo, vk::CommandBuffer cmdBuf,
                            const star::common::FrameTracker &ft);

    void recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &passInfo,
                        vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);
};
} // namespace renderer