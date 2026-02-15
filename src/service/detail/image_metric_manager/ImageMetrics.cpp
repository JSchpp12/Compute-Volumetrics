#include "service/detail/image_metric_manager/ImageMetrics.hpp"

#include <nlohmann/json.hpp>

namespace image_metric_manager
{
ImageMetrics::ImageMetrics(std::string imageFileName, double averageVisibilityDistance)
    : m_imageFileName(std::move(imageFileName)), m_averageVisisbilityDistance(std::move(averageVisibilityDistance))
{
}

std::string ImageMetrics::toJsonDump() const
{
    nlohmann::json data;
    data["file_name"] = m_imageFileName;
    data["visibility_distance"] = m_averageVisisbilityDistance;
    data["fog_type"] = "marched";

    return data.dump();
}
} // namespace image_metric_manager