#include "TerrainInfoFile.hpp"

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
		float top = float(images[i]["bounding_box"]["north"]);
		float bottom = float(images[i]["bounding_box"]["south"]);
		float left = float(images[i]["bounding_box"]["west"]);
		float right = float(images[i]["bounding_box"]["east"]);

		this->parsedInfo[i] = TerrainInfo{
			glm::vec2(right, bottom),
			glm::vec2(left, top),
			std::string(images[i]["height_file"]),
			std::string(images[i]["texture_file"])
		};
	}
}