#pragma once

#include "FogControlInfo.hpp"
#include "FogType.hpp"
#include "TerrainRenderingType.hpp"
#include "TerrainShapeInfo.hpp"
#include "Volume.hpp"
#include "service/detail/image_metric_manager/RayDistanceMetrics.hpp"
#include "service/detail/image_metric_manager/RayDistanceStats.hpp"
#include "service/detail/image_metric_manager/RayMaskFiles.hpp"
#include "service/detail/image_metric_manager/VolumeInfo.hpp"

#include <starlight/common/entities/Light.hpp>

#include <string>

namespace service::image_metric_manager
{
class ImageMetrics
{
  public:
    ImageMetrics(const star::Light &mainLight, const VolumeInfo &volumeInfo, const FogInfo &controlInfo,
                 const glm::vec3 &camPosition, const glm::vec3 &camLookDir, const std::string &fileName,
                 const RayDistanceMetrics &distanceMetrics, std::string_view terrainName, std::string_view volumeName,
                 Fog::Type type, const TerrainShapeInfo &terrainShapeInfo, TerrainRenderingType renderingType,
                 const RayMaskFiles &rayMaskFiles);

    std::string toJsonDump() const;

  private:
    const star::Light &m_mainLight;
    const VolumeInfo &m_volumeInfo;
    const FogInfo &m_controlInfo;
    const glm::vec3 &m_camPosition;
    const glm::vec3 &m_camLookDir;
    const std::string &m_imageFileName;
    const RayDistanceMetrics &m_distanceMetrics;
    std::string_view m_terrainName;
    std::string_view m_volumeName;
    Fog::Type m_type;
    const TerrainShapeInfo &m_terrainShapeInfo;
    TerrainRenderingType m_terrainRenderingType;
    RayMaskFiles m_rayMaskFiles;
};
} // namespace service::image_metric_manager
