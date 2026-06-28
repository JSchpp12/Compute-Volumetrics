#include "service/detail/image_metric_manager/VisibilityDistanceInfo_json.hpp"
#include "service/detail/image_metric_manager/VisibilityDistanceInfo.hpp"
#include "service/detail/image_metric_manager/RayVisibilityMetrics_json.hpp"

namespace service::image_metric_manager
{

void to_json(nlohmann::json &j, const VisibilityDistanceInfo &v)
{
    if (v.rayMetrics.has_value())
    {
        nlohmann::json rayJson;
        to_json(rayJson, v.rayMetrics.value());
        j["ray_metrics"] = rayJson;
    }
    if (v.simpleDistance.has_value())
    {
        j["simple_distance"] = v.simpleDistance.value();
    }
}

void from_json(const nlohmann::json &j, VisibilityDistanceInfo &v)
{
    if (j.contains("ray_metrics"))
    {
        RayVisibilityMetrics metrics;
        from_json(j["ray_metrics"], metrics);
        v.rayMetrics = metrics;
    }
    else if (j.contains("includingInvalidRays"))
    {
        RayVisibilityMetrics metrics;
        from_json(j, metrics);
        v.rayMetrics = metrics;
    }

    if (j.contains("simple_distance"))
    {
        v.simpleDistance = j["simple_distance"].get<double>();
    }
}

} // namespace service::image_metric_manager