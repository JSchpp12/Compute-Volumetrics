#include "TerrainGrid.hpp"

#include <cmath>
#include <iostream>

#include "MathHelpers.hpp"

void TerrainGrid::add(const std::string &heightFile, const std::string &textureFile, const glm::vec2 &upperLeft,
                      const glm::vec2 &lowerRight)
{
    this->chunkInfos.emplace_back(heightFile, textureFile, upperLeft, lowerRight);
}

std::vector<TerrainChunk> TerrainGrid::getFinalizedChunks()
{
    std::set<float> xBins = createXBins(this->chunkInfos);
    std::set<float> yBins = createYBins(this->chunkInfos);

    auto grid = createGrid(xBins, yBins, this->chunkInfos);

    // need to get the size of one chunk
    float width =
        std::abs(grid[0][0].chunkInfo.value().lowerRight.x) - std::abs(grid[0][0].chunkInfo.value().upperLeft.x);
    float height =
        std::abs(grid[0][0].chunkInfo.value().upperLeft.y) - std::abs(grid[0][0].chunkInfo.value().lowerRight.y);

    auto centerIndex = std::floor(grid.size() / 2);
    auto &centerOfGrid = grid[std::floor(grid.size() / 2)][std::floor(grid[0].size() / 2)].chunkInfo.value();
    // float centerHeight =
    // TerrainChunk::getCenterHeightFromGDAL(centerOfGrid.heightFile);
    float centerHeight = 0.0f;
    const glm::dvec3 centerOfTerrainGrid =
        glm::dvec3{centerOfGrid.getCenter().x, centerOfGrid.getCenter().y, centerHeight};

    std::vector<TerrainChunk> chunks;
    for (int i = 0; i < grid.size(); i++)
    {
        for (int j = 0; j < grid[i].size(); j++)
        {
            if (grid[i][j].chunkInfo.has_value())
            {
                auto info = grid[i][j].chunkInfo.value();

                // chunks.push_back(TerrainChunk(
                //     info.heightFile,
                //     info.textureFile,
                //     info.upperLeft,
                //     info.lowerRight,
                //     centerOfTerrainGrid));
            }
        }
    }

    // focus at grid[2][2]
    return chunks;
}

std::vector<std::vector<TerrainGrid::Space>> TerrainGrid::createGrid(const std::set<float> &binsX,
                                                                     const std::set<float> &binsY,
                                                                     const std::vector<ChunkInfo> &chunkInfo)
{
    std::vector<std::vector<Space>> grid =
        std::vector<std::vector<Space>>(binsY.size(), std::vector<Space>(binsX.size()));

    for (auto &info : chunkInfo)
    {
        size_t xCenter = getIndexOfValue(binsX, roundFloat(info.getCenter().x));
        size_t yCenter = getIndexOfValue(binsY, roundFloat(info.getCenter().y));

        assert(yCenter < grid.size() && xCenter < grid[0].size());
        assert(!grid[yCenter][xCenter].chunkInfo && "Something is already here");

        grid[yCenter][xCenter].chunkInfo = info;
    }

    return grid;
}

std::set<float> TerrainGrid::createXBins(const std::vector<ChunkInfo> &chunkInfo)
{
    std::set<float> bins;

    for (auto &info : chunkInfo)
    {
        bins.insert(roundFloat(info.getCenter().x)); // insert the rounded x-coordinate of the center into the set
    }

    return bins;
}

std::set<float> TerrainGrid::createYBins(const std::vector<ChunkInfo> &chunkInfo)
{
    std::set<float> bins;

    for (auto &info : chunkInfo)
    {
        bins.insert(roundFloat(info.getCenter().y)); // insert the rounded y-coordinate of the center into the set
    }

    return bins;
}

float TerrainGrid::roundFloat(const float &value)
{
    float mult = std::pow(10.0, 2);
    return std::round(value * mult) / mult;
}

size_t TerrainGrid::getIndexOfValue(const std::set<float> &bin, const float &value)
{
    float roundedValue = roundFloat(value);

    auto it = bin.find(roundedValue);

    if (it == bin.end())
    {
        throw std::runtime_error("Value not found in set");
    }

    return std::distance(bin.begin(), it);
}