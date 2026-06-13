#pragma once

#include <nlohmann/json.hpp>

namespace service::image_metric_manager
{
struct RayDistanceStats;
void to_json(nlohmann::json &j, const RayDistanceStats &v);
void from_json(const nlohmann::json &j, RayDistanceStats &v);
} // namespace service::image_metric_manager
