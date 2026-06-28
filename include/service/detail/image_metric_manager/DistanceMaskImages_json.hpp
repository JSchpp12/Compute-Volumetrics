#pragma once

#include <nlohmann/json.hpp>

namespace service::image_metric_manager
{
struct DistanceMaskImages;
void to_json(nlohmann::json &j, const DistanceMaskImages &v);
void from_json(const nlohmann::json &j, DistanceMaskImages &v);
} // namespace service::image_metric_manager