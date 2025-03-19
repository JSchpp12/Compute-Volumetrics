#pragma once

#include <glm/glm.hpp>

#include <string>
#include <array>
#include <vector>
#include <limits>
#include <optional>
#include <memory>

class TerrainGrid {

public:
    void add(const std::string& heightFile, const std::string& textureFile, const glm::vec2& upperLeft, const glm::vec2& lowerRight);

private:
    enum class Direction {
        north = 0,
        north_east = 1,
        east = 2,
        south_east = 3,
        south = 4,
        south_west = 5,
        west = 6,
        north_west = 7
    };

    struct ChunkInfo {
        ChunkInfo(const std::string& heightFile, const std::string& textureFile, const glm::vec2& upperLeft, const glm::vec2& lowerRight)
            : heightFile(heightFile), textureFile(textureFile), upperLeft(upperLeft), lowerRight(lowerRight) {}

        const std::string heightFile, textureFile;
        const glm::vec2 upperLeft, lowerRight;
    };

    struct Space {
        Space() = default;
        Space(const std::string& heightFile, const std::string& textureFile, const glm::vec2& upperLeft, const glm::vec2& lowerRight)
        : chunk(std::make_unique<ChunkInfo>(heightFile, textureFile, upperLeft, lowerRight)) {}
        ~Space() = default;

        std::unique_ptr<ChunkInfo> chunk = nullptr;
        std::array<Space*, 8> neighbors = std::array<Space*, 8>();
    };

    std::vector<std::unique_ptr<Space>> spaceStorage;

    static std::optional<std::pair<Space*, const Direction>> findInsertParent(Space* newSpace, Space* parent, Space* currentSearchSpace, const Direction* previousDirection);

    static bool isOnThisSideOfMain(const ChunkInfo& main, const ChunkInfo& secondary, const Direction& dir);

    static bool areDoubleClose(const double& valA, const double& valB, const double& epsilon = 0.001f);

    // void moveLineInDirection(Space* mainSpace, const Direction& lineLayoutDirection, const Direction& moveDirection);

    static Direction getOppositeDirection(const Direction& direction);

    Space& createNewSpace(const std::string& heightFile, const std::string& textureFile, const glm::vec2& upperLeft, const glm::vec2& lowerRight);

    void insert(Space* parentSpace, Space* newSpace, const Direction& direction);

    void moveSpace(Space* spaceToMove, const Direction& direction);
};