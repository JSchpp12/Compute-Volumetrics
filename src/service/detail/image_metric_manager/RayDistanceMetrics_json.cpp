#include "service/detail/image_metric_manager/RayDistanceMetrics_json.hpp"
#include "service/detail/image_metric_manager/RayDistanceMetrics.hpp"
#include "service/detail/image_metric_manager/RayDistanceStats_json.hpp"

namespace service::image_metric_manager
{

void to_json(nlohmann::json &j, const RayDistanceMetrics &v)
{
    nlohmann::json inc;
    to_json(inc, v.includingInvalidRays);
    nlohmann::json exc;
    to_json(exc, v.excludingInvalidRays);

    j = nlohmann::json{{"includingInvalidRays", inc}, {"excludingInvalidRays", exc}};
}

void from_json(const nlohmann::json &j, RayDistanceMetrics &v)
{
    // Use existing defaults if sections are absent
    from_json(j["includingInvalidRays"], v.includingInvalidRays);
    from_json(j["excludingInvalidRays"], v.excludingInvalidRays);
}

} // namespace service::image_metric_manager
