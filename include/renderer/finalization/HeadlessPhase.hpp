#pragma once

#include <starlight/core/renderer/HeadlessRenderPhase.hpp>

namespace renderer::finalization
{
class HeadlessPhaseProvider;

/// Finalization headless phase. Extends the starlight HeadlessRenderPhase --
/// which already owns the per-frame timeline semaphores, the TriggerPass submit
/// in frameUpdate, the GPU timeline wait before recording, and the edge-based
/// submission override -- and adds the finalization-specific color/depth image
/// layout transitions that bracket the render pass. The render-to color image
/// is left in TransferSrcOptimal by the previous frame's screenshot copy and
/// must return to ColorAttachmentOptimal; depth is transitioned out of
/// Undefined on the first frame. Setup lives on HeadlessPhaseProvider.
class HeadlessPhase : public star::core::renderer::HeadlessRenderPhase
{
  public:
    HeadlessPhase() = default;
    virtual ~HeadlessPhase() = default;

    HeadlessPhase(const HeadlessPhase &) = delete;
    HeadlessPhase &operator=(const HeadlessPhase &) = delete;
    HeadlessPhase(HeadlessPhase &&) = delete;
    HeadlessPhase &operator=(HeadlessPhase &&) = delete;

    virtual void recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer,
                                             const star::common::FrameTracker &ft) override;

    virtual void recordPostRenderingCalls(vk::CommandBuffer &commandBuffer,
                                          const star::common::FrameTracker &ft) override;

  private:
    void addMemoryBarriersPre(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft) const;

    void addMemoryBarriersPost(vk::CommandBuffer cmdBuffer, const star::common::FrameTracker &ft) const;
};
} // namespace renderer::finalization
