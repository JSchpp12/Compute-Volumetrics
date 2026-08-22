#include "render_system/fog/policies/ShadowResourceResolutionPolicy.hpp"

#include <star_terrain/rendering/ShadowCasterInfo.hpp>

#include <glm/glm.hpp>
#include <starlight/core/json/glm_json.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <sstream>

namespace render_system::fog::policies
{

#pragma region Builder

static void LogTransmittanceMapCoverage(const star::terrain::rendering::ShadowCasterInfo &shadowCalculator,
                                        const std::array<int, 3> &targetResolution) noexcept
{

    const auto bounds = shadowCalculator.getLightCameraFrustumInfo();
    int viewDiameter = bounds.viewSphereRadius * 2;
    glm::vec3 texelCoverages{0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 3; i++)
    {
        texelCoverages[i] = viewDiameter / targetResolution[i];
    }

    std::ostringstream oss;
    oss << "Transmittance map resolution info: " << std::endl;
    {
        nlohmann::json j = glm::vec3(targetResolution[0], targetResolution[1], targetResolution[2]);
        oss << "\tSize: " << j << std::endl;
    }
    {
        nlohmann::json j = texelCoverages;
        oss << "\tTexel coverage: " << j << std::endl;
    }

    star::core::logging::info(oss.str());
}

ShadowResourceResolutionPolicy ShadowResourceResolutionPolicy::Builder::build()
{
    // add optional logging information
    assert(m_mainWorldCamera != nullptr && m_shadowCastLightDir != nullptr);
    LogTransmittanceMapCoverage(star::terrain::rendering::ShadowCasterInfo{*m_mainWorldCamera, *m_shadowCastLightDir},
                                m_resolution.value());

    return ShadowResourceResolutionPolicy{m_resolution.value()};
}
#pragma endregion

#pragma region ShadowResourceResolutionPolicy

#pragma endregion
} // namespace render_system::fog::policies