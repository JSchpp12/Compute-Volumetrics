#pragma once

#include <starlight/core/renderer/HeadlessRenderPhase.hpp>

namespace renderer::finalization
{
class HeadlessPhaseProvider;

class HeadlessPhase : public star::core::renderer::HeadlessRenderPhase
{
  public:
    HeadlessPhase(const star::core::CommandBus &cmdBus, vk::Device device);
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
