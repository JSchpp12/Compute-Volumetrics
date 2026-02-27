#pragma once

#include "FogControlInfo.hpp"
#include "FogType.hpp"

#include <string>

namespace image_metric_manager
{
class ImageMetrics
{
  public:
    ImageMetrics(const FogInfo &controlInfo, const glm::vec3 &camPosition, const std::string &fileName,
                 const double &averageVisibilityDistance, Fog::Type type);

    std::string toJsonDump() const;

  private:
    const FogInfo &m_controlInfo;
    const glm::vec3 &m_camPosition;
    const std::string &m_imageFileName;
    const double &m_averageVisisbilityDistance;
    Fog::Type m_type;
};
} // namespace image_metric_manager