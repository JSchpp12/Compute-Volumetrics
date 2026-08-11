#pragma once

#include "core/renderer/FrameData.hpp"
#include "core/renderer/RenderingContext.hpp"

#include <starlight/core/renderer/RenderPhase.hpp>

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>
#include <star_common/IDeviceContext.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <vector>

class VolumeShadowRenderPhaseProvider;

/// Runtime half of the volume shadow renderer. Extends RenderPhase but records
/// a compute pass (no render targets/groups): it will own the shadow-mapping
/// pipelines, the shadow data buffers it produces, and the timeline semaphores,
/// and it samples the offscreen render-to images as input. All one-shot setup
/// -- queue-family lookup, DeclarePass, descriptor/command-buffer request --
/// lives on VolumeShadowRenderPhaseProvider::build. Per-frame recording,
/// frameUpdate, cleanup, and the submission override live here.
class VolumeShadowRenderPhase : public star::core::renderer::RenderPhase
{
  public:
    friend class VolumeShadowRenderPhaseProvider;

    explicit VolumeShadowRenderPhase(bool enableShadowCasting);
    virtual ~VolumeShadowRenderPhase() = default;

    VolumeShadowRenderPhase(const VolumeShadowRenderPhase &) = delete;
    VolumeShadowRenderPhase &operator=(const VolumeShadowRenderPhase &) = delete;
    VolumeShadowRenderPhase(VolumeShadowRenderPhase &&) = delete;
    VolumeShadowRenderPhase &operator=(VolumeShadowRenderPhase &&) = delete;

    virtual void frameUpdate(star::common::IDeviceContext &context) override;
    virtual void cleanupRender(star::common::IDeviceContext &context) override;
    virtual void recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                     const star::common::FrameTracker &frameTracker,
                                     const uint64_t &frameIndex) override;

    bool isRenderReady(star::core::device::DeviceContext &context);

    const std::vector<star::Handle> &getTimelineSemaphores() const
    {
        return m_timelineSemaphores;
    }

    void setShadowCastingEnabled(bool value)
    {
        m_shadowCastingEnabled = value;
    }
    bool isShadowCastingEnabled() const
    {
        return m_shadowCastingEnabled;
    }

  protected:
    virtual std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride>
    getSubmissionOverride() override;

  private:
    std::shared_ptr<star::core::renderer::FrameData> m_frameData;
    star::core::renderer::RenderingContext m_renderingContext = star::core::renderer::RenderingContext();
    std::vector<star::Handle> m_timelineSemaphores;
    uint32_t computeQueueFamilyIndex{0};
    bool m_shadowCastingEnabled = false;
    bool isReady = false;
    star::core::CommandBus *m_cmdBus{nullptr};
    vk::Device m_device{VK_NULL_HANDLE};

    void recordCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                        const uint64_t &frameIndex);
};
