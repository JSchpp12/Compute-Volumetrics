#pragma once

#include "service/detail/image_metric_manager/DistanceMaskImages.hpp"

#include <optional>
#include <string>

namespace service::image_metric_manager
{
struct ImageFilesInfo
{
    std::string sourceImageName;
    std::optional<DistanceMaskImages> distanceMaskImages;
};
} // namespace service::image_metric_manager