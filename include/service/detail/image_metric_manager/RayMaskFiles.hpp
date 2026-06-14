#pragma once

#include <string>

namespace service::image_metric_manager
{
struct RayMaskFiles
{
    std::string rayValidityName;
    std::string rayDistanceName;
    std::string rayNormalizedDistanceName;
};
} // namespace service::image_metric_manager