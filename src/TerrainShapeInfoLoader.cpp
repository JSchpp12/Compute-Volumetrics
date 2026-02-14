#include "TerrainShapeInfoLoader.hpp"

#include <starlight/common/helpers/FileHelpers.hpp>
#include <starlight/core/Exceptions.hpp>

#include <nlohmann/json.hpp>

#include <iostream>

TerrainShapeInfoLoader::TerrainShapeInfoLoader(std::string shapeFilePath) : m_shapeFilePath(std::move(shapeFilePath))
{
}

TerrainShapeInfo TerrainShapeInfoLoader::load() const
{
    if (!star::file_helpers::FileExists(m_shapeFilePath))
    {
        std::string msg = "Failed to load shape file: " + m_shapeFilePath;
        STAR_THROW(msg);
    }

    std::ifstream i(m_shapeFilePath);
    auto jData = nlohmann::json::parse(i);

    glm::dvec2 center = {std::stod(jData["center"]["lat"].get<std::string>()),
                         std::stod(jData["center"]["lon"].get<std::string>())};

    return {.viewDistance = 10, .center = std::move(center)};
}