#include "TerrainInfoFile.hpp"

#include "ConfigFile.hpp"

#include <nlohmann/json.hpp>

#include <iostream>

TerrainInfoFile::TerrainInfoFile(const std::string &pathToFile)
{
    loadFromFile(pathToFile);
}

void TerrainInfoFile::loadFromFile(const std::string &pathToFile)
{
    if (!star::file_helpers::FileExists(pathToFile))
        throw std::runtime_error("Terrain info file does not exist: " + pathToFile);

    std::ifstream i(pathToFile);
    nlohmann::json j = nlohmann::json::parse(i);

    auto &images = j["images"];
    this->fullHeightFilePath = j["full_terrain_file"];
    this->parsedInfo.resize(images.size());

    for (int i = 0; i < images.size(); i++)
    {
        this->parsedInfo[i] = TerrainInfo{
            glm::dvec2(double(images[i]["corners"]["NE"]["lat"]), double(images[i]["corners"]["NE"]["lon"])),
            glm::dvec2(double(images[i]["corners"]["SE"]["lat"]), double(images[i]["corners"]["SE"]["lon"])),
            glm::dvec2(double(images[i]["corners"]["SW"]["lat"]), double(images[i]["corners"]["SW"]["lon"])),
            glm::dvec2(double(images[i]["corners"]["NW"]["lat"]), double(images[i]["corners"]["NW"]["lon"])),
            glm::dvec2(double(images[i]["corners"]["center"]["lat"]), double(images[i]["corners"]["center"]["lon"])),
            std::string(images[i]["texture_name_no_extension"])};
    }
}