#include "MathHelpers.hpp"

glm::vec3 MathHelpers::toECEF(const glm::vec3& latLonAlt) {
    double a = 6378137.0;
    double e2 = 0.00669437999013;

    double latRad = glm::radians(latLonAlt.x);
    double lonRad = glm::radians(latLonAlt.y);

    double sinLatRad = std::sin(latRad);
    double e2sinLatSq = e2 * (sinLatRad * sinLatRad);

    double rn = a / std::sqrt(1.0 - e2sinLatSq);
    double R = (rn + latLonAlt.z) * std::cos(latRad);

    return glm::vec3{
        R * std::cos(lonRad),
        R * std::sin(lonRad),
        (rn * (1.0 - e2) + latLonAlt.z) * std::sin(latRad)
    };
}