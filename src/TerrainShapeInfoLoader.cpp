#include "TerrainShapeInfoLoader.hpp"

#include <starlight/command/FileIO/ReadFromFile.hpp>
#include <starlight/common/helpers/FileHelpers.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/job/tasks/IOTask.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

std::future<TerrainShapeInfo> TerrainShapeInfoLoader::SubmitForRead(std::filesystem::path filePath,
                                                                    const star::core::CommandBus &cmdBus)
{
    TerrainShapeInfoLoader shapeLoader{};
    auto future = shapeLoader.getFuture();

    auto readPayload = star::job::tasks::io::ReadPayload<TerrainShapeInfoLoader>{
        .filePath = std::move(filePath), .readFunction = std::move(shapeLoader)};
    auto readTask = star::job::tasks::io::CreateReadTask(std::move(readPayload));
    auto readCmd = star::command::file_io::ReadFromFile{std::move(readTask)};

    cmdBus.submit(readCmd);
    return future;
}

int TerrainShapeInfoLoader::operator()(const std::filesystem::path &filePath)
{
    auto data = load(filePath.string());
    m_shapeInfo.set_value(std::move(data));

    return 0;
}

TerrainShapeInfo TerrainShapeInfoLoader::load(const std::string &filePath) const
{
    if (!star::file_helpers::FileExists(filePath))
    {
        std::string msg = "Shape file does not exist: " + filePath;
        STAR_THROW(msg);
    }

    std::ifstream i(filePath);
    auto jData = nlohmann::json::parse(i);

    glm::dvec2 center = {std::stod(jData["center"]["lat"].get<std::string>()),
                         std::stod(jData["center"]["lon"].get<std::string>())};

    return {.viewDistance = 10, .center = std::move(center)};
}