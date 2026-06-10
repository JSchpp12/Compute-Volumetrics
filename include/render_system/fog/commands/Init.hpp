#pragma once

#include "render_system/fog/DispatchInfo.hpp"
#include "render_system/fog/PassInfo.hpp"

#include <star_common/FrameTracker.hpp>

#include <vulkan/vulkan.hpp>

namespace render_system::fog::commands
{
class Init
{
  public:
    struct OptionalClearBuffer
    {
        vk::Buffer buffer{VK_NULL_HANDLE};
        uint32_t fillValue{0};
    };

    Init(const vk::Extent2D &screenResolution, bool needsMemoryBarrierProtectFromPreviousDispatch = false);

    void recordCommands(const DispatchInfo &dInfo, const PassPipelineInfo &pipeInfo, vk::CommandBuffer cmdBuffer,
                        const star::common::FrameTracker &ft, std::span<OptionalClearBuffer> optionalClears = {});
    void setAdditionalClears(std::span<const OptionalClearBuffer> clears);

  private:
    static constexpr uint32_t MaxAdditionalClears = 2;

    std::array<OptionalClearBuffer, MaxAdditionalClears> m_additionalClears{};
    std::array<uint32_t, 2> m_workgroupSize{0, 0};
    uint32_t m_additionalClearCount{0};
    bool m_needsMemoryBarrierProtectFromPreviousDispatch{false};
};
} // namespace render_system::fog::commands