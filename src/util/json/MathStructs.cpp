#include "util/json/MathStructs.hpp"

namespace util
{
void to_json(nlohmann::json &jData, const glm::vec3 &data)
{
    jData["x"] = data.x; 
    jData["y"] = data.y; 
    jData["z"] = data.z; 
}

void from_json(const nlohmann::json& jData, glm::vec3 &data)
{
    data.x = jData["x"]; 
    data.y = jData["y"]; 
    data.z = jData["z"];
}
}