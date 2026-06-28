#pragma once

#include "FogControlInfo.hpp"
#include "FogType.hpp"
#include "TerrainRenderingType.hpp"
#include "Volume.hpp"
#include "service/detail/image_metric_manager/ImageFilesInfo.hpp"
#include "service/detail/image_metric_manager/VisibilityDistanceInfo.hpp"
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
                 const glm::vec3 &camPosition, const glm::vec3 &camLookDir,
                 const VisibilityDistanceInfo &distanceMetrics, std::string_view terrainName,
                 std::string_view volumeName, Fog::Type type, const star::terrain::CoverageInfo &terrainShapeInfo,
                 TerrainRenderingType renderingType, const ImageFilesInfo &imageFilesInfo);

    std::string toJsonDump() const;

  private:
    const star::Light &m_mainLight;
    const VolumeInfo &m_volumeInfo;
    const FogInfo &m_controlInfo;
    const glm::vec3 &m_camPosition;
    const glm::vec3 &m_camLookDir;
    const VisibilityDistanceInfo &m_distanceMetrics;
    std::string_view m_terrainName;
    std::string_view m_volumeName;
    Fog::Type m_type;
    const star::terrain::CoverageInfo &m_terrainShapeInfo;
    TerrainRenderingType m_terrainRenderingType;
    ImageFilesInfo m_imageFilesInfo;
};
} // namespace service::image_metric_manager