#include "service/detail/image_metric_manager/ImageMetrics_json.hpp"
#include "service/detail/image_metric_manager/ImageMetrics.hpp"

#include "FogInfo_json.hpp"
#include "FogType.hpp"
#include "service/detail/image_metric_manager/ImageFilesInfo_json.hpp"
#include "service/detail/image_metric_manager/VisibilityDistanceInfo_json.hpp"
#include "service/detail/image_metric_manager/VolumeInfo.hpp"
#include "service/detail/image_metric_manager/VolumeInfo_json.hpp"
#include <star_terrain/file_data/CoverageInfo_json.hpp>
#include <starlight/common/entities/Light_json.hpp>
#include <starlight/core/json/glm_json.hpp>

namespace service::image_metric_manager
{
void from_json(const nlohmann::json &j, ImageMetrics &d)
{
    from_json(j.at("image_files"), d.imageFilesInfo);
    j.at("camera_position").get_to(d.camPosition);
    j.at("camera_look_dir").get_to(d.camLookDir);
    from_json(j.at("distance_metrics"), d.distanceMetrics);
    d.type = Fog::TypeFromString(j.at("fog_type").get<std::string>());
    from_json(j.at("fog_params"), d.controlInfo);

    const auto &volNameNode = j.at("fog_volume_name");
    if (!volNameNode.is_null())
        d.volumeName = volNameNode.get<std::string>();
    else
        d.volumeName.clear();

    from_json(j.at("light"), d.mainLight);
    from_json(j.at("terrain_shape"), d.terrainShapeInfo);
    d.terrainName = j.at("terrain_name").get<std::string>();
    d.terrainRenderingType = star::terrain::rendering::fromString(j.at("terrain_shape_type").get<std::string>());
    from_json(j.at("volume_info"), d.volumeInfo);
}

void to_json(nlohmann::json &j, const ImageMetrics &d)
{
    nlohmann::json data;
    data["image_files"] = d.imageFilesInfo;
    data["camera_position"] = d.camPosition;
    data["camera_look_dir"] = d.camLookDir;
    data["distance_metrics"] = d.distanceMetrics;
    data["fog_type"] = Fog::TypeToString(d.type);
    data["fog_params"] = d.controlInfo;

    if ((d.type == Fog::Type::sMarched) && !d.volumeName.empty())
        data["fog_volume_name"] = d.volumeName;
    else
        data["fog_volume_name"] = nullptr;

    data["light"] = d.mainLight;
    data["terrain_shape"] = d.terrainShapeInfo;
    data["terrain_name"] = d.terrainName;
    data["terrain_shape_type"] = toString(d.terrainRenderingType);
    data["volume_info"] = d.volumeInfo;

    j = std::move(data);
}
} // namespace service::image_metric_manager