#pragma once

#include <glm/glm.hpp>

#include <array>
#include <optional>

namespace star
{
class StarCamera;
}

namespace render_system::fog::policies
{
class ShadowResourceResolutionPolicy
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
        Builder &setResolution(std::array<int, 3> resolution)
        {
            m_resolution = std::move(resolution);
            return *this;
        }
        ShadowResourceResolutionPolicy build();

      private:
        std::optional<std::array<int, 3>> m_resolution{std::nullopt};
        const glm::vec3 *m_shadowCastLightDir{nullptr};
        const star::StarCamera *m_mainWorldCamera{nullptr};
    };

    std::array<int, 3> getTransmittanceResolution() const noexcept
    {
        return m_resolution;
    }

  private:
    std::array<int, 3> m_resolution;

    explicit ShadowResourceResolutionPolicy(std::array<int, 3> resolution) : m_resolution(std::move(resolution))
    {
    }
};
} // namespace render_system::fog::policies