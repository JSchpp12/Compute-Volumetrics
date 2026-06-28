#pragma once

#include "service/detail/image_metric_manager/ImageMetrics.hpp"
#include "service/detail/image_metric_manager/SharedBufferHandle.hpp"

#include <starlight/virtual/StarCamera.hpp>

#include <filesystem>
#include <memory>

namespace service::image_metric_manager
{
class FileWriteFunction
{
  public:
    FileWriteFunction() = default;
    FileWriteFunction(std::shared_ptr<SharedBufferHandle> bufferHandle, const star::StarCamera &camera,
                      const Volume &volume, star::Light light, std::string terrainName,
                      star::terrain::CoverageInfo terrainShapeInfo, star::terrain::rendering::Type terrainRenderingType,
                      std::string volumeName, ImageFilesInfo imageFilesInfo);

    void write(const std::filesystem::path &path);

    int operator()(const std::filesystem::path &filePath);

  private:
    struct MetricWriteData
    {
        std::shared_ptr<SharedBufferHandle> bufferHandle;
        ImageMetrics capturedMetricInfo;
    };
    std::unique_ptr<MetricWriteData> m_data = nullptr;

    VisibilityDistanceInfo calculateDistanceMetrics() const;
};
} // namespace service::image_metric_manager