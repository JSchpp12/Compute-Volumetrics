#pragma once

#include "FogControlInfo.hpp"
#include "FogType.hpp"
#include "TerrainRenderingType.hpp"
#include "service/detail/image_metric_manager/VolumeInfo.hpp"

#include <star_terrain/file_data/coverage_info/CoverageInfo.hpp>

#include <starlight/common/entities/Light.hpp>

#include <string>

namespace service::image_metric_manager
{
class ImageMetrics
{
  public:
    ImageMetrics(const star::Light &mainLight, const VolumeInfo &volumeInfo, const FogInfo &controlInfo,
                 const glm::vec3 &camPosition, const glm::vec3 &camLookDir, const std::string &fileName,
                 const double &averageVisibilityDistance, std::string_view terrainName, std::string_view volumeName,
                 Fog::Type type, const star::terrain::CoverageInfo &terrainShapeInfo,
                 TerrainRenderingType renderingType);

    std::string toJsonDump() const;

  private:
    const star::Light &m_mainLight;
    const VolumeInfo &m_volumeInfo;
    const FogInfo &m_controlInfo;
    const glm::vec3 &m_camPosition;
    const glm::vec3 &m_camLookDir;
    const std::string &m_imageFileName;
    const double &m_averageVisisbilityDistance;
    std::string_view m_terrainName;
    std::string_view m_volumeName;
    Fog::Type m_type;
    const star::terrain::CoverageInfo &m_terrainShapeInfo;
    TerrainRenderingType m_terrainRenderingType;
};
} // namespace service::image_metric_manager