#pragma once

#include <starlight/core/renderer/DefaultRenderPhaseProvider.hpp>

#include <memory>
#include <vector>

class OffscreenRenderPhaseProvider : public star::core::renderer::DefaultRenderPhaseProvider
{
  public:
    OffscreenRenderPhaseProvider(star::core::device::DeviceContext &context,
                                 std::vector<std::shared_ptr<star::StarObject>> objects,
                                 std::shared_ptr<std::vector<star::Light>> lights,
                                 std::shared_ptr<star::StarCamera> camera);

    virtual ~OffscreenRenderPhaseProvider() = default;

    OffscreenRenderPhaseProvider(const OffscreenRenderPhaseProvider &) = delete;
    OffscreenRenderPhaseProvider &operator=(const OffscreenRenderPhaseProvider &) = delete;
    OffscreenRenderPhaseProvider(OffscreenRenderPhaseProvider &&) = default;
    OffscreenRenderPhaseProvider &operator=(OffscreenRenderPhaseProvider &&) = default;

    virtual std::unique_ptr<star::core::renderer::RenderPhase> build(
        star::core::device::DeviceContext &context, star::core::renderer::RenderPhaseRegistry &phases) override;
};