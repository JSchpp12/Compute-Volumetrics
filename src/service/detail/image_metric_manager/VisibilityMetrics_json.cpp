#include "service/detail/image_metric_manager/VisibilityMetrics_json.hpp"
#include "service/detail/image_metric_manager/VisibilityMetrics.hpp"

namespace service::image_metric_manager
{
void to_json(nlohmann::json &j, const VisibilityMetrics &v)
{
    j = nlohmann::json{{"minimum", v.minimum}, {"average", v.average}};
}

void from_json(const nlohmann::json &j, VisibilityMetrics &v)
{
    j.at("average").get_to(v.average);
    j.at("minimum").get_to(v.minimum);
}
} // namespace service::image_metric_manager
