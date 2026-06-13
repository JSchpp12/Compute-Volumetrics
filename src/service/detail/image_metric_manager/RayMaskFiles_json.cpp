#include "service/detail/image_metric_manager/RayMaskFiles_json.hpp"
#include "service/detail/image_metric_manager/RayMaskFiles.hpp"

namespace service::image_metric_manager
{
void to_json(nlohmann::json &j, const RayMaskFiles &v)
{
    j = nlohmann::json{{"ray_validity", v.rayValidity},
                       {"ray_distance", v.rayDistance},
                       {"ray_normalized_distance", v.rayNormalizedDistance}};
}

void from_json(const nlohmann::json &j, RayMaskFiles &v)
{
    j.at("ray_validity").get_to(v.rayValidity);
    j.at("ray_distance").get_to(v.rayDistance);
    j.at("ray_normalized_distance").get_to(v.rayNormalizedDistance);
}
} // namespace service::image_metric_manager