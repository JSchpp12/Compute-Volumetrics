#include "render_system/FogDispatcherBuilder.hpp"

namespace render_system
{

FogDispatcherBuilder &render_system::FogDispatcherBuilder::setOffscreenRenderer(OffscreenRenderer *offscreenRenderer)
{
    m_offscreenRenderer = offscreenRenderer;
    return *this;
}

FogDispatcherBuilder &render_system::FogDispatcherBuilder::setVolumeRenderer(VolumeRenderer *volumeRenderer)
{
    m_volumeRenderer = volumeRenderer;
    return *this;
}

FogDispatcherBuilder &render_system::FogDispatcherBuilder::setTargetScreenResolution(uint32_t height, uint32_t width)
{
    m_screenDimensions[0] = std::move(width);
    m_screenDimensions[1] = std::move(height);
    return *this;
}

FogDispatcherBuilder &render_system::FogDispatcherBuilder::setWorkgroupSize(uint32_t height, uint32_t width)
{
    m_workgroupDimensions[0] = std::move(width);
    m_workgroupDimensions[1] = std::move(height);
    return *this;
}

FogDispatcherBuilder &render_system::FogDispatcherBuilder::setNumOfDispatchSlices(uint32_t numSlices)
{
    m_numShaderSlicesPerFrame = std::move(numSlices);
    return *this;
}

FogDispatcher render_system::FogDispatcherBuilder::build()
{
    
}
} // namespace render_system