#pragma once

#include <glm/glm.hpp>

class MathHelpers {
public:
	static glm::vec3 toECEF(const glm::vec3& latLonAlt);

};