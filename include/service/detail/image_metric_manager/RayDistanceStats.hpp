#pragma once

namespace service::image_metric_manager
{

struct RayDistanceStats
{
    double average;
    double minimum;
    double median{};
    int rayCount;
};
} // namespace service::image_metric_manager
