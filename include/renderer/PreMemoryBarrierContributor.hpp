#pragma once

#include "renderer/PreMemoryBarrierDifferentFamilies.hpp"
#include "renderer/VolumePassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

#include <variant>

namespace renderer
{

class PreMemoryBarrierContributor
{
    std::variant<PreMemoryBarrierDifferentFamilies> m_approach;

  public:
    explicit PreMemoryBarrierContributor(PreMemoryBarrierDifferentFamilies approach) : m_approach(std::move(approach))
    {
    }

    void recordPreCommands(const VolumePassInfo &vInfo, vk::CommandBuffer cmdBuf,
                           const star::common::FrameTracker &ft);
};
} // namespace renderer