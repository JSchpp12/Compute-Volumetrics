#include "service/detail/image_metric_manager/RayVisibilityMetrics_json.hpp"
#include "service/detail/image_metric_manager/RayVisibilityMetrics.hpp"
#include "service/detail/image_metric_manager/RayDistanceStats_json.hpp"

namespace service::image_metric_manager
{

void to_json(nlohmann::json &j, const RayVisibilityMetrics &v)
{
    nlohmann::json inc;
    to_json(inc, v.includingInvalidRays);
    nlohmann::json exc;
    to_json(exc, v.excludingInvalidRays);

    j = nlohmann::json{{"includingInvalidRays", inc}, {"excludingInvalidRays", exc}};
}

void from_json(const nlohmann::json &j, RayVisibilityMetrics &v)
{
    from_json(j["includingInvalidRays"], v.includingInvalidRays);
    from_json(j["excludingInvalidRays"], v.excludingInvalidRays);
}

} // namespace service::image_metric_manager