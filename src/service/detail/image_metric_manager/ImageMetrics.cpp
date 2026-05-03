#include "service/detail/image_metric_manager/ImageMetrics.hpp"

#include "TerrainShapeInfo_json.hpp"
#include "util/json/FogInfoStruct.hpp"
#include "util/json/MathStructs.hpp"

#include <starlight/common/entities/Light_json.hpp>

namespace service::image_metric_manager
{

ImageMetrics::ImageMetrics(const star::Light &mainLight, const FogInfo &controlInfo, const glm::vec3 &camPosition,
                           const glm::vec3 &camLookDir, const std::string &imageFileName,
                           const double &averageVisibilityDistance, std::string_view terrainName,
                           std::string_view volumeName, Fog::Type type, const TerrainShapeInfo &terrainShapeInfo,
                           TerrainRenderingType renderingType)
    : m_mainLight(mainLight), m_controlInfo(controlInfo), m_camPosition(camPosition), m_camLookDir(camLookDir),
      m_imageFileName(imageFileName), m_averageVisisbilityDistance(averageVisibilityDistance),
      m_terrainName(terrainName), m_volumeName(volumeName), m_type(type), m_terrainShapeInfo(terrainShapeInfo),
      m_terrainRenderingType(renderingType)
{
}

std::string ImageMetrics::toJsonDump() const
{
    nlohmann::json data;
    data["file_name"] = m_imageFileName;
    {
        nlohmann::json cData;
        util::to_json(cData, m_camPosition);
        data["camera_position"] = cData;
        util::to_json(cData, m_camLookDir);
        data["camera_look_dir"] = cData;
    }

    data["visibility_distance"] = m_averageVisisbilityDistance;
    data["fog_type"] = Fog::TypeToString(m_type);

    nlohmann::json fogData;
    util::to_json(fogData, m_controlInfo);

    data["fog_params"] = fogData;

    if ((m_type == Fog::Type::sMarched) && !m_volumeName.empty())
        data["fog_volume_name"] = m_volumeName;
    else
        data["fog_volume_name"] = nullptr; 

    data["light"] = m_mainLight;
    data["terrain_shape"] = m_terrainShapeInfo;
    data["terrain_name"] = m_terrainName;
    data["terrain_shape_type"] = toString(m_terrainRenderingType);

    std::ostringstream oss;
    oss << std::setw(4) << data;

    return oss.str();
}
} // namespace service::image_metric_manager