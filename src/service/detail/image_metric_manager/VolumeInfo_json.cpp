#include "service/detail/image_metric_manager/VolumeInfo_json.hpp"
#include "service/detail/image_metric_manager/VolumeInfo.hpp"

#include <starlight/core/json/glm_json.hpp>

namespace service::image_metric_manager
{
void to_json(nlohmann::json &j, const VolumeInfo &v)
{
    j = nlohmann::json{{"position", v.position}, {"rotation", v.rotation}, {"scale", v.scale}};
}

void from_json(const nlohmann::json &j, VolumeInfo &v)
{
    j.at("position").get_to(v.position); 
    j.at("rotation").get_to(v.rotation);
    j.at("scale").get_to(v.scale);
}
}

