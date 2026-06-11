#pragma once

#include "service/detail/image_metric_manager/RayDistanceStats.hpp"

namespace service::image_metric_manager
{
struct RayDistanceMetrics
{
    RayDistanceStats includingInvalidRays; 
    RayDistanceStats excludingInvalidRays;
    size_t rayCount = 0;
};
} // namespace service::image_metric_manager
