#pragma once

#include "OffscreenRenderer.hpp"
#include "renderer/VolumeRenderer.hpp"

namespace render_system
{

class FogDispatcherBuilder
{
  public:
    explicit FogDispatcherBuilder(star::core::device::DeviceContext *ctx);

    FogDispatcherBuilder &setOffscreenRenderer(OffscreenRenderer *offscreenRenderer);
    FogDispatcherBuilder &setVolumeRenderer(VolumeRenderer *volumeRenderer);
    FogDispatcherBuilder &setTargetScreenResolution(uint32_t height, uint32_t width);
    FogDispatcherBuilder &setWorkgroupSize(uint32_t height, uint32_t width);
    FogDispatcherBuilder &setNumOfDispatchSlices(uint32_t numSlices);
    // FogDispatcher build();

  private:
    uint32_t m_screenDimensions[2]{0, 0};
    uint32_t m_workgroupDimensions[2]{0, 0};
    uint32_t m_numWorkgroupsNeeded{0};
    uint32_t m_numShaderSlicesPerFrame{0};
    OffscreenRenderer *m_offscreenRenderer{nullptr};
    VolumeRenderer *m_volumeRenderer{nullptr};
};
} // namespace render_system