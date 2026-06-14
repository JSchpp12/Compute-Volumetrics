#pragma once

#include <nlohmann/json.hpp>

namespace service::image_metric_manager
{
struct VisibilityDistanceInfo;
void to_json(nlohmann::json &j, const VisibilityDistanceInfo &v);
void from_json(const nlohmann::json &j, VisibilityDistanceInfo &v);
} // namespace service::image_metric_manager
