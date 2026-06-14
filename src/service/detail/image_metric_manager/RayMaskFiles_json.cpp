#include "service/detail/image_metric_manager/RayMaskFiles_json.hpp"
#include "service/detail/image_metric_manager/RayMaskFiles.hpp"

namespace service::image_metric_manager
{
void to_json(nlohmann::json &j, const RayMaskFiles &v)
{
    j = nlohmann::json{{"ray_validity_name", v.rayValidityName},
                       {"ray_distance_name", v.rayDistanceName},
                       {"ray_normalized_distance_name", v.rayNormalizedDistanceName}};
}

void from_json(const nlohmann::json &j, RayMaskFiles &v)
{
    j.at("ray_validity_name").get_to(v.rayValidityName);
    j.at("ray_distance_name").get_to(v.rayDistanceName);
    j.at("ray_normalized_distance_name").get_to(v.rayNormalizedDistanceName);
}
} // namespace service::image_metric_manager