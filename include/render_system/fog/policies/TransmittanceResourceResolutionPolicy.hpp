#pragma once

#include "render_system/fog/policies/TransmittancePlan.hpp"

#include <glm/glm.hpp>

#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace star
{
class StarCamera;
}

namespace render_system::fog::policies
{
/// @brief Resolution policy describing the transmittance render resources: their shapes (map resolution, noise
/// resolution) and the variant table for the transmittance precompute pipelines.
class TransmittanceResourceResolutionPolicy
{
  public:
    class Builder
    {
      public:
        Builder() = default;
        Builder &setMainWorldCamera(const star::StarCamera &camera)
        {
            m_mainWorldCamera = &camera;
            return *this;
        }
        Builder &setShadowCastLightDir(const glm::vec3 &shadowCastLightDir)
        {
            m_shadowCastLightDir = &shadowCastLightDir;
            return *this;
        }
        Builder &setTransmittanceMapResolution(std::array<uint32_t, 3> transmittanceMapResolution)
        {
            m_transmittanceMapResolution = std::move(transmittanceMapResolution);
            return *this;
        }
        /// @brief Variant table for the transmittance precompute pipelines; the first entry is the default.
        Builder &setTransmittanceVariants(std::vector<TransmittanceVariant> transmittanceVariants)
        {
            m_transmittanceVariants = std::move(transmittanceVariants);
            return *this;
        }
        TransmittanceResourceResolutionPolicy build();

      private:
        std::optional<std::array<uint32_t, 3>> m_transmittanceMapResolution{std::nullopt};
        std::optional<std::vector<TransmittanceVariant>> m_transmittanceVariants{std::nullopt};
        const glm::vec3 *m_shadowCastLightDir{nullptr};
        const star::StarCamera *m_mainWorldCamera{nullptr};
    };

    TransmittancePlan transmittance;

  private:
    explicit TransmittanceResourceResolutionPolicy(TransmittancePlan transmittance)
        : transmittance(std::move(transmittance))
    {
    }
};
} // namespace render_system::fog::policies
