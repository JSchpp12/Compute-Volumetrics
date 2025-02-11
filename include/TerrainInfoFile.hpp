#pragma once 

#include "FileHelpers.hpp"

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <stdio.h>

class TerrainInfoFile {
public:
	struct TerrainInfo {
		glm::vec2 lowerRight = glm::vec2();
		glm::vec2 upperLeft = glm::vec2();
		std::string heightFile = std::string(); 
		std::string surfaceTexture = std::string();

		TerrainInfo(const glm::vec2& lowerRight, const glm::vec2& upperLeft, 
			const std::string& heightFile, const std::string& surfaceTexture)
			: lowerRight(lowerRight), upperLeft(upperLeft), heightFile(heightFile), surfaceTexture(surfaceTexture) {
		}

		TerrainInfo() = default; 
	};

	TerrainInfoFile(const std::string& pathToFile); 

	const std::vector<TerrainInfo>& infos() const { return this->parsedInfo; }

private:
	/// @brief Load terrain info from json file
	/// @param pathToFile full path to the file to be read
	void loadFromFile(const std::string& pathToFile); 
	
	std::vector<TerrainInfo> parsedInfo = std::vector<TerrainInfo>(); 

};