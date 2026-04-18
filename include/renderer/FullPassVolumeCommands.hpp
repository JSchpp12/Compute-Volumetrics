#pragma once

#include "renderer/VolumeComputeCommands.hpp"

class VolumeRenderer;

namespace renderer
{
class FullPassVolumeCommands
{
    VolumeComputeCommands m_mainCmds;

  public:
    explicit FullPassVolumeCommands(VolumeComputeCommands mainCmds) : m_mainCmds(std::move(mainCmds))
    {
    }

    void recordPreCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);

    void recordPostCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);

    void recordCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);
};
} // namespace renderer