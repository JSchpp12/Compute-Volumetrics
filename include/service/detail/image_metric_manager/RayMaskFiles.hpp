#pragma once

#include <string>

namespace service::image_metric_manager
{
struct RayMaskFiles
{
    std::string rayValidity;
    std::string rayDistance;
    std::string rayNormalizedDistance;
};
} // namespace service::image_metric_manager