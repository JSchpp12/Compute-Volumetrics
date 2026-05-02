#pragma once

#include <nlohmann/json.hpp>

struct TerrainShapeInfo; 

void to_json(nlohmann::json &j, const TerrainShapeInfo &data); 
void from_json(const nlohmann::json &j, TerrainShapeInfo &data);