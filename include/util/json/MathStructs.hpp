#pragma once

#include <nlohmann/json.hpp>
#include <glm/vec3.hpp>

namespace util
{
void to_json(nlohmann::json &jData, const glm::vec3 &data); 
void from_json(const nlohmann::json &jData, glm::vec3 &data); 
}