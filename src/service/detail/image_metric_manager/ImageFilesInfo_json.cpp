#include "service/detail/image_metric_manager/ImageFilesInfo_json.hpp"
#include "service/detail/image_metric_manager/ImageFilesInfo.hpp"
#include "service/detail/image_metric_manager/DistanceMaskImages_json.hpp"

namespace service::image_metric_manager
{
void to_json(nlohmann::json &j, const ImageFilesInfo &v)
{
    j = nlohmann::json{{"file_name", v.sourceImageName}};
    if (v.distanceMaskImages.has_value())
    {
        nlohmann::json maskJson;
        to_json(maskJson, v.distanceMaskImages.value());
        j["distance_mask_images"] = maskJson;
    }
}

void from_json(const nlohmann::json &j, ImageFilesInfo &v)
{
    j.at("file_name").get_to(v.sourceImageName);
    if (j.contains("distance_mask_images"))
    {
        DistanceMaskImages masks;
        from_json(j["distance_mask_images"], masks);
        v.distanceMaskImages = masks;
    }
    else if (j.contains("ray_validity_name"))
    {
        DistanceMaskImages masks;
        from_json(j, masks);
        v.distanceMaskImages = masks;
    }
}
} // namespace service::image_metric_manager