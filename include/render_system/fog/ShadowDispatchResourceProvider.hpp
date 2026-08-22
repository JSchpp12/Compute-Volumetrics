#pragma once

#include "render_system/fog/policies/ShadowResourceResolutionPolicy.hpp"

#include <starlight/core/renderer/DescriptorRecipe.hpp>

#include <memory>

namespace star
{
class StarCamera;
class Light;
} // namespace star

namespace star::core::device
{
class DeviceContext;
} // namespace star::core::device

namespace star::core::renderer
{
class FrameData;
} // namespace star::core::renderer

namespace render_system::fog
{
class ShadowDispatchResourceProvider
{
  public:
    explicit ShadowDispatchResourceProvider(policies::ShadowResourceResolutionPolicy resPolicy);

    /// @brief Add required resources to the frameData for use in render phase provider for the volume
    /// @param c
    /// @param frameData
    /// @return true if success, false on failure
    bool addResourcesTo(star::core::device::DeviceContext &c,
                        star::core::renderer::FrameData &frameData) const noexcept;

  private:
    policies::ShadowResourceResolutionPolicy m_resPolicy;

    bool addTransmittanceMaps(star::core::device::DeviceContext &c,
                              star::core::renderer::FrameData &frameData) const noexcept;
};
} // namespace render_system::fog