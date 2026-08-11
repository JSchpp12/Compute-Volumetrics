#pragma once

#include "StarCamera.hpp"
#include "core/renderer/FrameData.hpp"

#include <starlight/core/renderer/IRenderPhaseProvider.hpp>

#include <star_common/Handle.hpp>

#include <memory>
#include <string>

/// Builds a VolumeShadowRenderPhase. Holds the shadow setup recipe -- the
/// shared offscreen FrameData, the offscreen render phase whose render-to
/// images the shadow pass samples, the camera, and the initial shadow config
/// the application threads in before the phase is built. build() runs the
/// cold-path setup on the phase and returns it.
class VolumeShadowRenderPhaseProvider : public star::core::renderer::IRenderPhaseProvider
{
  public:
    VolumeShadowRenderPhaseProvider(std::shared_ptr<star::core::renderer::FrameData> frameData,
                                    star::Handle offscreenPhaseHandle, star::StarCamera *camera,
                                    bool enableShadowCasting);

    virtual ~VolumeShadowRenderPhaseProvider() = default;

    VolumeShadowRenderPhaseProvider(const VolumeShadowRenderPhaseProvider &) = delete;
    VolumeShadowRenderPhaseProvider &operator=(const VolumeShadowRenderPhaseProvider &) = delete;
    VolumeShadowRenderPhaseProvider(VolumeShadowRenderPhaseProvider &&) = default;
    VolumeShadowRenderPhaseProvider &operator=(VolumeShadowRenderPhaseProvider &&) = default;

    void setOffscreenPhaseHandle(star::Handle handle)
    {
        m_offscreenPhaseHandle = std::move(handle);
    }

    virtual std::unique_ptr<star::core::renderer::RenderPhase> build(
        star::core::device::DeviceContext &context, star::core::renderer::RenderPhaseRegistry &phases) override;

  private:
    std::shared_ptr<star::core::renderer::FrameData> m_frameData;
    star::Handle m_offscreenPhaseHandle{};
    star::StarCamera *m_camera{nullptr};
    bool m_enableShadowCasting = false;
};
