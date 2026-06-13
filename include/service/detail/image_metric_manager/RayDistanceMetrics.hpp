#pragma once

#include "service/detail/image_metric_manager/RayDistanceStats.hpp"

namespace service::image_metric_manager
{
struct RayDistanceMetrics
{
    RayDistanceStats includingInvalidRays; 
    RayDistanceStats excludingInvalidRays;};
} // namespace service::image_metric_manager
