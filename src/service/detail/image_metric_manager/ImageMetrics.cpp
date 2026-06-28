#include "service/detail/image_metric_manager/ImageMetrics.hpp"

#include "FogInfo_json.hpp"
#include "TerrainShapeInfo_json.hpp"
#include "service/detail/image_metric_manager/VisibilityDistanceInfo_json.hpp"
#include "service/detail/image_metric_manager/ImageFilesInfo_json.hpp"
#include "service/detail/image_metric_manager/VolumeInfo.hpp"
#include "service/detail/image_metric_manager/VolumeInfo_json.hpp"

#include <starlight/common/entities/Light_json.hpp>
#include <starlight/core/json/glm_json.hpp>

namespace service::image_metric_manager
{

ImageMetrics::ImageMetrics(const star::Light &mainLight, const VolumeInfo &volumeInfo, const FogInfo &controlInfo,
                           const glm::vec3 &camPosition, const glm::vec3 &camLookDir,
                           const VisibilityDistanceInfo &distanceMetrics, std::string_view terrainName,
                           std::string_view volumeName, Fog::Type type, const TerrainShapeInfo &terrainShapeInfo,
                           TerrainRenderingType renderingType, const ImageFilesInfo &imageFilesInfo)
    : m_mainLight(mainLight), m_volumeInfo(volumeInfo), m_controlInfo(controlInfo), m_camPosition(camPosition),
      m_camLookDir(camLookDir), m_distanceMetrics(distanceMetrics),
      m_terrainName(terrainName), m_volumeName(volumeName), m_type(type), m_terrainShapeInfo(terrainShapeInfo),
      m_terrainRenderingType(renderingType), m_imageFilesInfo(imageFilesInfo)
{
}

std::string ImageMetrics::toJsonDump() const
{
    nlohmann::json data;
    data["image_files"] = m_imageFilesInfo;
    data["camera_position"] = m_camPosition;
    data["camera_look_dir"] = m_camLookDir;
    data["distance_metrics"] = m_distanceMetrics;
    data["fog_type"] = Fog::TypeToString(m_type);
    data["fog_params"] = m_controlInfo;

    if ((m_type == Fog::Type::sMarched) && !m_volumeName.empty())
        data["fog_volume_name"] = m_volumeName;
    else
        data["fog_volume_name"] = nullptr;

    data["light"] = m_mainLight;
    data["terrain_shape"] = m_terrainShapeInfo;
    data["terrain_name"] = m_terrainName;
    data["terrain_shape_type"] = toString(m_terrainRenderingType);
    data["volume_info"] = m_volumeInfo;

    std::ostringstream oss;
    oss << std::setw(4) << data;

    return oss.str();
}
} // namespace service::image_metric_manager
