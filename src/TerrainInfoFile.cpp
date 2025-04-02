#include "TerrainInfoFile.hpp"

#include <nlohmann/json.hpp>

#include <iostream>

TerrainInfoFile::TerrainInfoFile(const std::string& pathToFile) {
	loadFromFile(pathToFile);
}

void TerrainInfoFile::loadFromFile(const std::string& pathToFile) {
	if (!star::FileHelpers::FileExists(pathToFile))
		throw std::runtime_error("Terrain info file does not exist: " + pathToFile);

	std::ifstream i(pathToFile); 
	nlohmann::json j = nlohmann::json::parse(i);

	auto& images = j["images"]; 
	this->parsedInfo.resize(images.size());

	for (int i = 0; i < images.size(); i++) {
		this->parsedInfo[i] = TerrainInfo{
			glm::dvec2(double(images[i]["corners"]["NE"]["lat"]), double(images[i]["corners"]["NE"]["lon"])),
			glm::dvec2(double(images[i]["corners"]["SE"]["lat"]), double(images[i]["corners"]["SE"]["lon"])),
			glm::dvec2(double(images[i]["corners"]["SW"]["lat"]), double(images[i]["corners"]["SW"]["lon"])),
			glm::dvec2(double(images[i]["corners"]["NW"]["lat"]), double(images[i]["corners"]["NW"]["lon"])),
			std::string(images[i]["height_file"]),
			std::string(images[i]["texture_file"])
		};
	}
}