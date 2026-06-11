#pragma once

#include <nlohmann/json.hpp>

namespace service::image_metric_manager
{
struct VisibilityMetrics;
void to_json(nlohmann::json &j, const VisibilityMetrics &v);
void from_json(const nlohmann::json &j, VisibilityMetrics &v);
} // namespace service::image_metric_manager
