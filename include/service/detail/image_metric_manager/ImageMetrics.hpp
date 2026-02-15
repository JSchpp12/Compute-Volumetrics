#pragma once

#include <string>

namespace image_metric_manager
{
class ImageMetrics
{
  public:
    ImageMetrics(std::string fileName, double averageVisibilityDistance);

    std::string toJsonDump() const;

  private:
    std::string m_imageFileName;
    double m_averageVisisbilityDistance;
};
} // namespace image_metric_manager