#pragma once

#include "renderer/PostMemoryBarrierContributor.hpp"
#include "renderer/PreMemoryBarrierContributor.hpp"
#include "renderer/VolumeComputeCommands.hpp"

#include <optional>

class VolumeRenderer;

namespace renderer
{
class FullPassVolumeCommands
{
    std::optional<PreMemoryBarrierContributor> m_preMemCmds{std::nullopt};
    std::optional<PostMemoryBarrierContributor> m_postMemCmds{std::nullopt};
    VolumeComputeCommands m_mainCmds;

  public:
    explicit FullPassVolumeCommands(VolumeComputeCommands mainCmds)
        : m_preMemCmds{std::nullopt}, m_postMemCmds{std::nullopt}, m_mainCmds(std::move(mainCmds))
    {
    }
    FullPassVolumeCommands(VolumeComputeCommands mainCmds, PreMemoryBarrierContributor preMemCmds)
        : m_preMemCmds(std::move(preMemCmds)), m_postMemCmds{std::nullopt}, m_mainCmds(std::move(mainCmds))
    {
    }
    FullPassVolumeCommands(VolumeComputeCommands mainCmds, PostMemoryBarrierContributor postMemCmds)
        : m_preMemCmds{std::nullopt}, m_postMemCmds(std::move(postMemCmds)), m_mainCmds(std::move(mainCmds))
    {
    }
    FullPassVolumeCommands(VolumeComputeCommands mainCmds, PreMemoryBarrierContributor preMemCmds,
                           PostMemoryBarrierContributor postMemCmds)
        : m_preMemCmds(std::move(preMemCmds)), m_postMemCmds(std::move(postMemCmds)), m_mainCmds(std::move(mainCmds))
    {
    }

    void recordPreCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);

    void recordPostCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);

    void recordCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);
};
} // namespace renderer