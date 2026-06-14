#pragma once

#include "service/detail/image_metric_manager/RayVisibilityMetrics.hpp"

#include <optional>

namespace service::image_metric_manager
{
struct VisibilityDistanceInfo
{
    std::optional<RayVisibilityMetrics> rayMetrics;
    std::optional<double> simpleDistance;
};
} // namespace service::image_metric_manager