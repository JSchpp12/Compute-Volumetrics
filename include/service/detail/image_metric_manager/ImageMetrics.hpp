#pragma once

#include "FogControlInfo.hpp"
#include "FogType.hpp"
#include "Volume.hpp"
#include "service/detail/image_metric_manager/ImageFilesInfo.hpp"
#include "service/detail/image_metric_manager/VisibilityDistanceInfo.hpp"
#include "service/detail/image_metric_manager/VolumeInfo.hpp"

#include <star_terrain/file_data/CoverageInfo.hpp>
#include <star_terrain/rendering/TerrainRenderingType.hpp>

#include <starlight/common/entities/Light.hpp>

#include <string>

namespace service::image_metric_manager
{
struct ImageMetrics
{
    star::Light mainLight;
    VolumeInfo volumeInfo;
    FogInfo controlInfo;
    glm::vec3 camPosition;
    glm::vec3 camLookDir;
    VisibilityDistanceInfo distanceMetrics;
    std::string terrainName;
    std::string volumeName;
    Fog::Type type;
    star::terrain::CoverageInfo terrainShapeInfo;
    star::terrain::rendering::Type terrainRenderingType;
    ImageFilesInfo imageFilesInfo;
};
} // namespace service::image_metric_manager