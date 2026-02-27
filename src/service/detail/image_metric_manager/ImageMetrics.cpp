#include "service/detail/image_metric_manager/ImageMetrics.hpp"

#include "util/json/FogInfoStruct.hpp"
#include "util/json/MathStructs.hpp"

namespace image_metric_manager
{

ImageMetrics::ImageMetrics(const FogInfo &controlInfo, const glm::vec3 &camPosition, const std::string &imageFileName,
                           const double &averageVisibilityDistance, Fog::Type type)
    : m_controlInfo(controlInfo), m_camPosition(camPosition), m_imageFileName(imageFileName),
      m_averageVisisbilityDistance(averageVisibilityDistance), m_type(type)
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
    }
    data["visibility_distance"] = m_averageVisisbilityDistance;
    data["fog_type"] = Fog::TypeToString(m_type);

    nlohmann::json fogData;
    util::to_json(fogData, m_controlInfo);

    data["fog_params"] = fogData;

    std::ostringstream oss;
    oss << std::setw(4) << data;

    return oss.str();
}
} // namespace image_metric_manager