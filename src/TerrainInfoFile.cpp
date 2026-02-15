#include "TerrainInfoFile.hpp"

#include <starlight/common/ConfigFile.hpp>
#include <starlight/common/helpers/FileHelpers.hpp>

#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

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

    for (size_t i = 0; i < images.size(); i++)
    {
        std::string name = images[i]["name"].get<std::string>();
        glm::dvec2 ne{std::stod(images[i]["bounds"]["northEast"]["lat"].get<std::string>()),
                      std::stod(images[i]["bounds"]["northEast"]["lon"].get<std::string>())};
        glm::dvec2 se{std::stod(images[i]["bounds"]["southEast"]["lat"].get<std::string>()),
                      std::stod(images[i]["bounds"]["southEast"]["lon"].get<std::string>())};
        glm::dvec2 sw{std::stod(images[i]["bounds"]["southWest"]["lat"].get<std::string>()),
                      std::stod(images[i]["bounds"]["southWest"]["lon"].get<std::string>())};
        glm::dvec2 nw{std::stod(images[i]["bounds"]["northWest"]["lat"].get<std::string>()),
                      std::stod(images[i]["bounds"]["northWest"]["lon"].get<std::string>())};
        glm::dvec2 center{std::stod(images[i]["bounds"]["center"]["lat"].get<std::string>()),
                          std::stod(images[i]["bounds"]["center"]["lon"].get<std::string>())};
        this->parsedInfo[i] =
            TerrainInfo{std::move(ne), std::move(se), std::move(sw), std::move(nw), std::move(center), std::move(name)};
    }
}