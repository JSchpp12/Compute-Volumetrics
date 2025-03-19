#include "TerrainGrid.hpp"

#include <cmath>
#include <stdexcept>
#include <assert.h>
#include <iostream>

void TerrainGrid::add(const std::string& heightFile, const std::string& textureFile, const glm::vec2& upperLeft, const glm::vec2& lowerRight){
    Space& newSpace = this->createNewSpace(heightFile, textureFile, upperLeft, lowerRight); 
    
    if (this->spaceStorage.size() == 1)
        return;
    
    
    std::optional<std::pair<Space*, const Direction>> parentOfInsert = findInsertParent(&newSpace, nullptr, this->spaceStorage[0].get(), nullptr); 

    if (!parentOfInsert.has_value())
        throw std::runtime_error("Failed to find insert position"); 

    auto& result = parentOfInsert.value();

    insert(result.first, &newSpace, result.second);
}

std::optional<std::pair<TerrainGrid::Space*, const TerrainGrid::Direction>> TerrainGrid::findInsertParent(Space* newSpace, Space* parent, Space* currentSearchSpace, const Direction* previousDirection){
    if (currentSearchSpace == nullptr || !currentSearchSpace->chunk){
        return std::make_pair(parent, *previousDirection);
    }

    for(int i = 0; i < 8; i++){
        Direction searchDirection = static_cast<Direction>(i);
        if (isOnThisSideOfMain(*currentSearchSpace->chunk, *newSpace->chunk, searchDirection)){
            Space* neighbor = currentSearchSpace->neighbors[i];

            //check if going back against direction of previous direction
            if (previousDirection == nullptr || searchDirection != getOppositeDirection(*previousDirection)){
                return findInsertParent(newSpace, currentSearchSpace, neighbor, &searchDirection);
            }else{
                return std::make_optional(std::make_pair(neighbor, static_cast<Direction>(i)));
            }
        }
    }

    return std::nullopt;
}

bool TerrainGrid::isOnThisSideOfMain(const ChunkInfo& mainPiece, const ChunkInfo& secondary, const Direction& dir){
    if ((dir == Direction::north
            && areDoubleClose(mainPiece.upperLeft.x, secondary.upperLeft.x)
            && areDoubleClose(mainPiece.lowerRight.x, secondary.lowerRight.x)
            && secondary.upperLeft.y > mainPiece.upperLeft.y
            && secondary.lowerRight.y > mainPiece.lowerRight.y)
       || (dir == Direction::north_east
            && mainPiece.upperLeft.y < secondary.upperLeft.y
            && mainPiece.lowerRight.y < secondary.lowerRight.y
            && mainPiece.upperLeft.x < secondary.upperLeft.x
            && mainPiece.lowerRight.x < secondary.lowerRight.x)
       || (dir == Direction::east
            && areDoubleClose(mainPiece.upperLeft.y, secondary.upperLeft.y)
            && areDoubleClose(mainPiece.lowerRight.y, secondary.lowerRight.y)
            && mainPiece.upperLeft.x < secondary.upperLeft.y
            && mainPiece.lowerRight.x < secondary.lowerRight.x)
       || (dir == Direction::south_east
            && mainPiece.upperLeft.y > secondary.upperLeft.y
            && mainPiece.lowerRight.y > secondary.lowerRight.y
            && mainPiece.upperLeft.x < secondary.upperLeft.x
            && mainPiece.lowerRight.x < secondary.lowerRight.x)
       || (dir == Direction::south 
            && areDoubleClose(mainPiece.upperLeft.x, secondary.upperLeft.x)
            && areDoubleClose(mainPiece.lowerRight.x, secondary.lowerRight.x)
            && mainPiece.upperLeft.y > secondary.upperLeft.y
            && mainPiece.lowerRight.y > secondary.lowerRight.y)
       || (dir == Direction::south_west
            && mainPiece.upperLeft.y > secondary.upperLeft.y
            && mainPiece.lowerRight.y > secondary.lowerRight.y
            && mainPiece.upperLeft.x > secondary.upperLeft.x
            && mainPiece.lowerRight.x > secondary.lowerRight.x)
       || (dir == Direction::west
           && areDoubleClose(mainPiece.upperLeft.y, secondary.upperLeft.y)
           && areDoubleClose(mainPiece.lowerRight.y, secondary.lowerRight.y)
           && mainPiece.upperLeft.x > secondary.upperLeft.x
           && mainPiece.lowerRight.x > secondary.lowerRight.x)
        || (dir == Direction::north_west
            && mainPiece.upperLeft.x > secondary.upperLeft.x 
            && mainPiece.lowerRight.x > secondary.lowerRight.x
            && mainPiece.upperLeft.y < secondary.upperLeft.y
            && mainPiece.lowerRight.y < secondary.lowerRight.y)){
        return true;
    }

    return false; 
}

bool TerrainGrid::areDoubleClose(const double& valA, const double& valB, const double& epsilon){
    return std::fabs(valA - valB) <= epsilon * std::max({1.0, std::fabs(valA), std::fabs(valB)});
}

// void TerrainGrid::moveLineInDirection(Space* mainSpace, const Direction& lineLayoutDirection, const Direction& moveDirection){
//     assert(mainSpace != nullptr); 
//     assert(moveDirection == Direction::north || moveDirection == Direction::east || moveDirection == Direction::south || moveDirection == Direction::west);
//     assert(lineLayoutDirection == Direction::north || lineLayoutDirection == Direction::east || lineLayoutDirection == Direction::south || lineLayoutDirection == Direction::west);

//     Space* focusSpace = mainSpace; 
//     std::vector<Space*> line = std::vector<Space*>();
//     while (focusSpace != nullptr){
//         line.push_back(focusSpace);
//         focusSpace = focusSpace->neighbors[static_cast<int>(lineLayoutDirection)];
//     }

//     for (Space* space : line){

//     }
// }

TerrainGrid::Direction TerrainGrid::getOppositeDirection(const Direction& direction){
    return static_cast<Direction>((static_cast<int>(direction)+4)%8);
}

TerrainGrid::Space& TerrainGrid::createNewSpace(const std::string& heightFile, const std::string& textureFile, const glm::vec2& upperLeft, const glm::vec2& lowerRight){
    this->spaceStorage.emplace_back(std::make_unique<Space>(heightFile, textureFile, upperLeft, lowerRight)); 

    return *this->spaceStorage.back();
}

void TerrainGrid::insert(Space* parentSpace, Space* newSpace, const Direction& direction){
    assert(parentSpace != nullptr && newSpace != nullptr);

    if (parentSpace->neighbors[static_cast<int>(direction)] != nullptr){
        moveSpace(parentSpace->neighbors[static_cast<int>(direction)], direction); 
    }

    parentSpace->neighbors[static_cast<int>(direction)] = newSpace; 
    newSpace->neighbors[static_cast<int>(getOppositeDirection(direction))] = parentSpace; 
}

void TerrainGrid::moveSpace(Space* spaceToMove, const Direction& direction){
    assert(spaceToMove != nullptr); 

    const Direction oppositeDirection = getOppositeDirection(direction);

    Space* newSpaceLocation = nullptr;
    //if there is a space in the direction we are moving, it needs to be moved
    if (spaceToMove->neighbors[static_cast<int>(direction)] != nullptr){ 
        //move the existing space
        moveSpace(spaceToMove->neighbors[static_cast<int>(direction)], direction);
        newSpaceLocation = spaceToMove->neighbors[static_cast<int>(direction)]; 
    }else{
        //create new space and mvoe the chunk data
        this->spaceStorage.push_back(std::make_unique<Space>()); 
        newSpaceLocation = this->spaceStorage.back().get(); 
        newSpaceLocation->chunk = std::move(spaceToMove->chunk); 
    }

    //update neighbor pointers
    for (int i = 0; i < 8; i++){
        if (i != static_cast<int>(getOppositeDirection(direction)) && spaceToMove->neighbors[i] != nullptr){
            spaceToMove->neighbors[static_cast<int>(getOppositeDirection(static_cast<Direction>(i)))] = newSpaceLocation; 
        }
    }

    Space* oldOppositeParent = spaceToMove->neighbors[static_cast<int>(getOppositeDirection(direction))];
    int nextA = static_cast<int>(direction) + 1 % 8; 
    int nextB = 0; 
    if (static_cast<int>(direction) == 0)
        nextB = 7; 
    else
        nextB = static_cast<int>(direction) - 1 % 8;

    if (oldOppositeParent != nullptr){
        oldOppositeParent->neighbors[nextA] = nullptr; 
        oldOppositeParent->neighbors[nextB] = nullptr; 
    }
}
