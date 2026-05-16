#pragma once

#include <nlohmann/json.hpp>
namespace service::image_metric_manager
{
struct VolumeInfo;
void to_json(nlohmann::json &j, const VolumeInfo &v);
void from_json(const nlohmann::json &j, VolumeInfo &v);
} // namespace service::image_metric_manager