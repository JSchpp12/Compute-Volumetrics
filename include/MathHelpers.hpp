#pragma once

#include <glm/glm.hpp>

class MathHelpers
{
  public:
    static glm::dvec3 toECEF(const double &lat, const double &lon, const double &alt);

    static glm::dmat3 getECEFToENUTransformation(const double &lat, const double &lon);

    static double feetToMeters(const double &feet);

    static double metersToFeet(const double &meters);
};