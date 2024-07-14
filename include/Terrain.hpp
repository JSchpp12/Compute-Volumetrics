#pragma once

#include "StarObject.hpp"
#include "TextureMaterial.hpp"

#include <gdal_priv.h>
#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>


class Terrain : public star::StarObject{
public:
	Terrain(const std::string& terrainDefPath, const std::string& texturePath, const glm::vec3& upperLeft, const glm::vec3& lowerRight)
		: terrainDefPath(terrainDefPath), texturePath(texturePath), 
		upperLeft(upperLeft), lowerRight(lowerRight) {};

protected:
	// Inherited via StarObject
	std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

	std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> loadGeometryBuffers(star::StarDevice& device) override;

private: 
	const std::string terrainDefPath, texturePath; 
	const glm::vec3 upperLeft, lowerRight; 

	static glm::vec3 toECEF(const glm::vec3& latLonAlt) {

        double a = 6378137.0;
        double e2 = 0.00669437999013;

        double latRad = glm::radians(latLonAlt.x);
        double lonRad = glm::radians(latLonAlt.y);

        double sinLatRad = std::sin(latRad);
        double e2sinLatSq = e2 * (sinLatRad * sinLatRad);

        double rn = a / std::sqrt(1.0 - e2sinLatSq);
        double R = (rn + latLonAlt.z) * std::cos(latRad);

        return glm::vec3{
            R* std::cos(lonRad),
            R* std::sin(lonRad),
            (rn * (1.0 - e2) + latLonAlt.z) * std::sin(latRad)
        };
	}
};