#include "service/detail/image_metric_manager/RayDistanceStats_json.hpp"
#include "service/detail/image_metric_manager/RayDistanceStats.hpp"

namespace service::image_metric_manager
{
void to_json(nlohmann::json &j, const RayDistanceStats &v)
{
    j = nlohmann::json{{"minimum", v.minimum}, {"average", v.average}, {"median", v.median}, {"rayCount", v.rayCount}};
}

void from_json(const nlohmann::json &j, RayDistanceStats &v)
{
    j.at("average").get_to(v.average);
    j.at("minimum").get_to(v.minimum);
    v.median = j.at("median").get<double>();
    j.at("rayCount").get_to(v.rayCount);
}
} // namespace service::image_metric_manager
