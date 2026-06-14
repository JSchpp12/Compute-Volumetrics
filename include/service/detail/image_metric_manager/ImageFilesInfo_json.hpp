#pragma once

#include <nlohmann/json.hpp>

namespace service::image_metric_manager
{
struct ImageFilesInfo;
void to_json(nlohmann::json &j, const ImageFilesInfo &v);
void from_json(const nlohmann::json &j, ImageFilesInfo &v);
} // namespace service::image_metric_manager