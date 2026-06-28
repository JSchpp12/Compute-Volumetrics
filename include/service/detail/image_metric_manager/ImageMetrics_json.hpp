#pragma once

#include <nlohmann/json.hpp>

namespace service::image_metric_manager
{

struct ImageMetrics;

void from_json(const nlohmann::json &j, ImageMetrics &d);

void to_json(nlohmann::json &j, const ImageMetrics &d);

} // namespace service::image_metric_manager