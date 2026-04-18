#pragma once

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

class VolumeRenderer;

namespace renderer
{
class VolumeComputeCommands
{
    VolumeRenderer *m_me{nullptr};

  public:
    explicit VolumeComputeCommands(VolumeRenderer *me) : m_me(me){};

    void recordCommands(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft);
};
} // namespace renderer