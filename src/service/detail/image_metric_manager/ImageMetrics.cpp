#include "service/detail/image_metric_manager/ImageMetrics.hpp"

#include <nlohmann/json.hpp>

namespace image_metric_manager
{

ImageMetrics::ImageMetrics(const FogInfo &controlInfo, const std::string &imageFileName, const double &averageVisibilityDistance,
                           Fog::Type type)
    : m_controlInfo(controlInfo), m_imageFileName(imageFileName),
      m_averageVisisbilityDistance(averageVisibilityDistance), m_type(type)
{
}

std::string ImageMetrics::toJsonDump() const
{
    nlohmann::json data;
    data["file_name"] = m_imageFileName;
    data["visibility_distance"] = m_averageVisisbilityDistance;
    data["fog_type"] = Fog::TypeToString(m_type);

    return data.dump();
}
} // namespace image_metric_manager