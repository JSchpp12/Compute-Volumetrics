#pragma once

#include "FogType.hpp"
#include "TerrainRenderingType.hpp"
#include "TerrainShapeInfo.hpp"
#include "Volume.hpp"
#include "service/detail/image_metric_manager/VisibilityDistanceInfo.hpp"
#include "service/detail/image_metric_manager/ImageFilesInfo.hpp"
#include "service/detail/image_metric_manager/SharedBufferHandle.hpp"
#include "service/detail/image_metric_manager/VolumeInfo.hpp"
#include "structs/FogInfo.hpp"

#include <starlight/common/entities/Light.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <memory>

namespace service::image_metric_manager
{
class FileWriteFunction
{
  public:
    FileWriteFunction() = default;
    FileWriteFunction(std::shared_ptr<SharedBufferHandle> bufferHandle, vk::Extent2D screenResolution,
                      const star::StarCamera &camera, const Volume &volume, star::Light light,
                      std::string terrainName, TerrainShapeInfo terrainShapeInfo,
                      TerrainRenderingType terrainRenderingType, std::string volumeName,
                      ImageFilesInfo imageFilesInfo);

    void write(const std::filesystem::path &path) const;

    int operator()(const std::filesystem::path &filePath);

  private:
    struct MetricWriteData
    {
        struct CameraInfo
        {
            glm::vec3 position;
            glm::vec3 lookDir;
        };

        std::shared_ptr<SharedBufferHandle> bufferHandle;
        std::string terrainName;
        std::string volumeName;
        vk::Extent2D screenResolution;
        CameraInfo cameraInfo;
        VolumeInfo volumeInfo;
        star::Light light;
        FogInfo controlInfo;
        Fog::Type type;
        TerrainShapeInfo shapeInfo;
        TerrainRenderingType terrainRenderingType;
        ImageFilesInfo imageFilesInfo;
    };
    std::unique_ptr<MetricWriteData> m_data = nullptr;

    VisibilityDistanceInfo calculateDistanceMetrics() const;
};
} // namespace service::image_metric_manager