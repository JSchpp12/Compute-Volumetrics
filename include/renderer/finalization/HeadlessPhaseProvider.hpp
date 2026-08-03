#pragma once

#include <starlight/core/renderer/HeadlessRenderPhaseProvider.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace renderer::finalization
{
/// Builds a HeadlessPhase for the headless finalization path. Shares the
/// offscreen renderer's FrameData (so the finalization records against the same
/// per-frame camera/light controllers) and waits at eAllCommands. build() runs
/// the shared base prep (buildCore) then the headless tail-setup
/// (prepareHeadlessPhase, inherited from HeadlessRenderPhaseProvider) on a
/// HeadlessPhase, which adds the finalization color/depth layout transitions.
class HeadlessPhaseProvider : public star::core::renderer::HeadlessRenderPhaseProvider
{
  public:
    HeadlessPhaseProvider(star::core::device::DeviceContext &context,
                          std::vector<std::shared_ptr<star::StarObject>> objects,
                          std::shared_ptr<star::core::renderer::FrameData> frameData,
                          vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eAllCommands);

    virtual ~HeadlessPhaseProvider() = default;

    HeadlessPhaseProvider(const HeadlessPhaseProvider &) = delete;
    HeadlessPhaseProvider &operator=(const HeadlessPhaseProvider &) = delete;
    HeadlessPhaseProvider(HeadlessPhaseProvider &&) = default;
    HeadlessPhaseProvider &operator=(HeadlessPhaseProvider &&) = default;

    virtual std::unique_ptr<star::core::renderer::RenderPhase>
    build(star::core::device::DeviceContext &context, star::core::renderer::RenderPhaseRegistry &phases) override;
};
} // namespace renderer::finalization
