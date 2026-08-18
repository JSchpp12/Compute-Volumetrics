#pragma once

#include <starlight/core/renderer/DefaultRenderPhase.hpp>
#include <starlight/core/CommandBus.hpp>

class OffscreenRenderPhaseProvider;

/// Runtime half of the offscreen renderer (extends DefaultRenderPhase). Adds
/// the compute<->graphics queue-ownership image transitions in
/// recordPreRenderPassCommands, the store-op depth attachment, the offscreen
/// submit override (timeline semaphores + neighbor edges), and the per-frame
/// wait. Setup (queue-family lookup, DeclarePass, timeline-semaphore creation)
/// lives on OffscreenRenderPhaseProvider.
class OffscreenRenderPhase : public star::core::renderer::DefaultRenderPhase
{
  public:
    OffscreenRenderPhase(const star::core::CommandBus &cmdBus, vk::Device device);
    virtual ~OffscreenRenderPhase() = default;

    OffscreenRenderPhase(const OffscreenRenderPhase &) = delete;
    OffscreenRenderPhase &operator=(const OffscreenRenderPhase &) = delete;
    OffscreenRenderPhase(OffscreenRenderPhase &&) = delete;
    OffscreenRenderPhase &operator=(OffscreenRenderPhase &&) = delete;

    virtual void recordPreRenderPassCommands(vk::CommandBuffer &buffer, const star::common::FrameTracker &ft) override;

    virtual void recordPostRenderingCalls(vk::CommandBuffer &buffer, const star::common::FrameTracker &ft) override;

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(
        const star::common::FrameTracker &frameTracker) override;

    virtual void recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                     const star::common::FrameTracker &frameInFlightIndex,
                                     const uint64_t &frameIndex) override;

    const std::vector<star::Handle> getTimelineSemaphroes() const
    {
        return m_timelineSemaphores;
    }

  protected:
    virtual void updateDependentData(star::core::device::DeviceContext &context) override;

  private:
    friend class OffscreenRenderPhaseProvider;

    uint32_t graphicsQueueFamilyIndex = 0;
    uint32_t computeQueueFamilyIndex = 0;
    uint32_t firstFramePassCounter = 0;
    bool isFirstPass = true;
    std::vector<star::Handle> m_timelineSemaphores;

    vk::Device m_device{VK_NULL_HANDLE};
    const star::core::CommandBus *m_cmdBus{nullptr};

    std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> getSubmissionOverride()
        override;

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(
        const star::common::FrameTracker &frameTracker) override;
};
