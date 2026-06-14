#pragma once

#include <nlohmann/json.hpp>

namespace service::image_metric_manager
{
struct RayVisibilityMetrics;
void to_json(nlohmann::json &j, const RayVisibilityMetrics &v);
void from_json(const nlohmann::json &j, RayVisibilityMetrics &v);
} // namespace service::image_metric_manager