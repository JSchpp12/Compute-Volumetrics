#pragma once

#include "render_system/FogDispatchInfo.hpp"
#include "renderer/PostMemoryBarrierContributor.hpp"
#include "renderer/PreMemoryBarrierContributor.hpp"
#include "renderer/VolumeComputeCommandsContributor.hpp"
#include "renderer/VolumePassInfo.hpp"

#include <starlight/wrappers/graphics/StarCommandBuffer.hpp>

#include <optional>

class VolumeRenderer;

namespace renderer
{

class FullPassVolumeCommands
{
    std::optional<PreMemoryBarrierContributor> m_preMemCmds{std::nullopt};
    std::optional<PostMemoryBarrierContributor> m_postMemCmds{std::nullopt};
    std::optional<VolumeComputeCommandsContributor> m_mainCmds{std::nullopt};

  public:
    FullPassVolumeCommands() = default;
    FullPassVolumeCommands(VolumeComputeCommandsContributor mainCmds)
        : m_preMemCmds{std::nullopt}, m_postMemCmds{std::nullopt}, m_mainCmds(std::move(mainCmds))
    {
    }
    FullPassVolumeCommands(VolumeComputeCommandsContributor mainCmds, PreMemoryBarrierContributor preMemCmds)
        : m_preMemCmds(std::move(preMemCmds)), m_postMemCmds{std::nullopt}, m_mainCmds(std::move(mainCmds))
    {
    }
    FullPassVolumeCommands(VolumeComputeCommandsContributor mainCmds, PostMemoryBarrierContributor postMemCmds)
        : m_preMemCmds{std::nullopt}, m_postMemCmds(std::move(postMemCmds)), m_mainCmds(std::move(mainCmds))
    {
    }
    FullPassVolumeCommands(VolumeComputeCommandsContributor mainCmds, PreMemoryBarrierContributor preMemCmds,
                           PostMemoryBarrierContributor postMemCmds)
        : m_preMemCmds(std::move(preMemCmds)), m_postMemCmds(std::move(postMemCmds)), m_mainCmds(std::move(mainCmds))
    {
    }

    void recordPreCommands(const VolumePassInfo &tInfo, vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);

    void recordPostCommands(const VolumePassInfo &tInfo, vk::CommandBuffer cmdBuf,
                            const star::common::FrameTracker &ft);

    void recordCommands(const render_system::FogDispatchInfo &dInfo, const renderer::VolumePassPipelineInfo &passInfo,
                        vk::CommandBuffer cmdBuf, const star::common::FrameTracker &ft);
};
} // namespace renderer