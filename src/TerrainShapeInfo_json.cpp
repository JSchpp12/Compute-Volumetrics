#include "TerrainShapeInfo.hpp"
#include "TerrainShapeInfo_json.hpp"

#include <starlight/core/json/glm_json.hpp>

void to_json(nlohmann::json &j, const TerrainShapeInfo &data)
{
    j["view_distance"] = data.viewDistance; 
    j["center"] = data.center; 
}

void from_json(const nlohmann::json &j, TerrainShapeInfo &data)
{
    data.center = j["center"].get<glm::dvec2>(); 
    data.viewDistance = j["view_distance"].get<int>();
}
