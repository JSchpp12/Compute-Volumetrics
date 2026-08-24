#pragma once

#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/policies/ShadowDepthAcquirePolicy.hpp"
#include "render_system/fog/struct/BarrierBatch.hpp"

#include <variant>

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog::commands::transmittance
{
/// Acquires the shadow depth image (graphics -> compute, depth attachment -> shader read)
struct ShadowDepthAcquire
{
    ShadowDepthAcquirePolicy policy;
    void build(const PassInfo &info, BarrierBatch &batch) const noexcept;
};

using PreMemoryPolicy = std::variant<ShadowDepthAcquire>;

class PreMemoryBarrierRecorder
{
    PreMemoryPolicy m_policy;

  public:
    explicit PreMemoryBarrierRecorder(PreMemoryPolicy policy) : m_policy(std::move(policy))
    {
    }

    void recordCommands(const PassInfo &vInfo, const star::common::FrameTracker &ft,
                        vk::CommandBuffer cmdBuf) const noexcept;
};
} // namespace render_system::fog::commands::transmittance