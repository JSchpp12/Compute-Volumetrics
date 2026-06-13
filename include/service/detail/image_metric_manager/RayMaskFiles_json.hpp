#pragma once

#include <nlohmann/json.hpp>

namespace service::image_metric_manager
{
struct RayMaskFiles;
void to_json(nlohmann::json &j, const RayMaskFiles &v);
void from_json(const nlohmann::json &j, RayMaskFiles &v);
} // namespace service::image_metric_manager