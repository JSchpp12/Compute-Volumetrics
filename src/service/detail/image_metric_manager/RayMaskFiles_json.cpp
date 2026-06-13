#include "service/detail/image_metric_manager/RayMaskFiles_json.hpp"
#include "service/detail/image_metric_manager/RayMaskFiles.hpp"

namespace service::image_metric_manager
{
void to_json(nlohmann::json &j, const RayMaskFiles &v)
{
    j = nlohmann::json{{"ray_validity", v.rayValidity}, {"ray_distance", v.rayDistance}};
}

void from_json(const nlohmann::json &j, RayMaskFiles &v)
{
    j.at("ray_validity").get_to(v.rayValidity);
    j.at("ray_distance").get_to(v.rayDistance);
}
} // namespace service::image_metric_manager