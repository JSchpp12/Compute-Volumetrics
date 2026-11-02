#include "render_system/fog/policies/TransmittanceResourceResolutionPolicy.hpp"

#include <star_terrain/rendering/ShadowFrustumCalculator.hpp>

#include <glm/glm.hpp>
#include <starlight/core/json/glm_json.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <sstream>

namespace render_system::fog::policies
{
static void LogTransmittanceMapCoverage(const star::terrain::rendering::ShadowFrustumCalculator &shadowCalculator,
                                        const std::array<uint32_t, 3> &targetResolution) noexcept
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

TransmittanceResourceResolutionPolicy TransmittanceResourceResolutionPolicy::Builder::build()
{
    // add optional logging information
    assert(m_mainWorldCamera != nullptr && m_shadowCastLightDir != nullptr);
    assert(m_transmittanceMapResolution.has_value() && "The transmittance map resolution must be provided");
    assert(m_transmittanceVariants.has_value() && "The transmittance variant table must be provided");
    LogTransmittanceMapCoverage(
        star::terrain::rendering::ShadowFrustumCalculator{*m_mainWorldCamera, *m_shadowCastLightDir},
        m_transmittanceMapResolution.value());

    return TransmittanceResourceResolutionPolicy{TransmittancePlan::Builder()
                                              .setTransmittanceMapResolution(m_transmittanceMapResolution.value())
                                              .setVariants(std::move(m_transmittanceVariants.value()))
                                              .build()};
}
} // namespace render_system::fog::policies