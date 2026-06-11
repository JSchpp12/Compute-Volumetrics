#pragma once

#include <nlohmann/json.hpp>

namespace service::image_metric_manager
{
struct RayDistanceMetrics;
void to_json(nlohmann::json &j, const RayDistanceMetrics &v);
void from_json(const nlohmann::json &j, RayDistanceMetrics &v);
} // namespace service::image_metric_manager
