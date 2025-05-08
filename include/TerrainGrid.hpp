#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "TerrainChunk.hpp"

class TerrainGrid
{
  public:
    void add(const std::string &heightFile, const std::string &textureFile, const glm::vec2 &upperLeft,
             const glm::vec2 &lowerRight);

    std::vector<TerrainChunk> getFinalizedChunks();

  private:
    enum class Direction
    {
        north = 0,
        north_east = 1,
        east = 2,
        south_east = 3,
        south = 4,
        south_west = 5,
        west = 6,
        north_west = 7
    };

    struct ChunkInfo
    {
        ChunkInfo(const ChunkInfo &o)
            : heightFile(o.heightFile), textureFile(o.textureFile), upperLeft(o.upperLeft), lowerRight(o.lowerRight)
        {
        }
        ChunkInfo(const std::string &heightFile, const std::string &textureFile, const glm::vec2 &upperLeft,
                  const glm::vec2 &lowerRight)
            : heightFile(heightFile), textureFile(textureFile), upperLeft(upperLeft), lowerRight(lowerRight)
        {
        }

        glm::dvec2 getCenter() const
        {
            double x = (upperLeft.x + lowerRight.x) / 2.0f;
            double y = (upperLeft.y + lowerRight.y) / 2.0f;
            return glm::dvec2{x, y};
        }
        std::string heightFile, textureFile;
        glm::vec2 upperLeft, lowerRight;
    };

    struct Space
    {
        Space() = default;

        std::optional<ChunkInfo> chunkInfo = std::nullopt;
    };

    std::vector<ChunkInfo> chunkInfos = std::vector<ChunkInfo>();

    std::set<float> createXBins(const std::vector<ChunkInfo> &chunkInfos);

    std::set<float> createYBins(const std::vector<ChunkInfo> &chunkInfos);

    static std::vector<std::vector<Space>> createGrid(const std::set<float> &binsX, const std::set<float> &binsY,
                                                      const std::vector<ChunkInfo> &chunkInfos);

    // void moveLineInDirection(Space* mainSpace, const Direction&
    // lineLayoutDirection, const Direction& moveDirection);

    static Direction getOppositeDirection(const Direction &direction);

    static float roundFloat(const float &value);

    static size_t getIndexOfValue(const std::set<float> &bin, const float &value);
};