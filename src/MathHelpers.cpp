#include "MathHelpers.hpp"

glm::dvec3 MathHelpers::toECEF(const double& lat, const double& lon, const double& alt) {
    double a = 6378137.0;
    double e2 = 0.00669437999013;

    double latRad = glm::radians(lat);
    double lonRad = glm::radians(lon);

    double sinLatRad = std::sin(latRad);
    double e2sinLatSq = e2 * (sinLatRad * sinLatRad);

    double rn = a / std::sqrt((double)1.0f - e2sinLatSq);
    double R = (rn + alt) * std::cos(latRad);

    return glm::dvec3{
        R * std::cos(lonRad),
        R * std::sin(lonRad),
        (rn * ((double)1.0f - e2) + alt) * std::sin(latRad)
    };
}

glm::dmat3 MathHelpers::getECEFToENUTransformation(const double& lat, const double& lon){

    const double phi = glm::radians(lat);  
    const double lambda = glm::radians(lon);

    const double sinPhi = glm::sin(phi);
    const double cosPhi = glm::cos(phi); 
    const double sinLambda = glm::sin(lambda);
    const double cosLambda = glm::cos(lambda);

    return glm::dmat3{
        -sinLambda, -cosLambda * sinPhi, cosLambda * cosPhi,
        cosLambda, -sinLambda * sinPhi, sinLambda * cosPhi,
        0.0, cosPhi, sinPhi,
    };
} 

double MathHelpers::feetToMeters(const double& feet){
    return feet * 0.3048;
}

double MathHelpers::metersToFeet(const double& meters){
    return meters / 0.3048;
}