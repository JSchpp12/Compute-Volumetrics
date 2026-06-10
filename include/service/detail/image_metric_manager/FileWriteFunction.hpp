#pragma once

#include "structs/FogInfo.hpp"
#include "FogType.hpp"
#include "TerrainRenderingType.hpp"
#include "TerrainShapeInfo.hpp"
#include "Volume.hpp"
#include "service/detail/image_metric_manager/HostVisibleStorage.hpp"
#include "service/detail/image_metric_manager/VolumeInfo.hpp"

#include <starlight/common/entities/Light.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <vulkan/vulkan.hpp>

namespace service::image_metric_manager
{
class FileWriteFunction
{
  public:
    FileWriteFunction() = default;
    FileWriteFunction(const star::StarCamera &camera, const Volume &volume, star::Light light, star::Handle buffer,
                      vk::Device vkDevice, vk::Semaphore done, uint64_t copyToHostBufferDoneValue,
                      HostVisibleStorage *storage, std::string terrainName, TerrainShapeInfo terrainShapeInfo,
                      TerrainRenderingType terrainRenderingType, std::string volumeName);

    void write(const std::filesystem::path &path) const;

    int operator()(const std::filesystem::path &filePath);

  private:

    struct ImageWriteData
    {
        struct CameraInfo
        {
            glm::vec3 position;
            glm::vec3 lookDir;
        };

        std::string terrainName;
        std::string volumeName;
        CameraInfo cameraInfo;
        VolumeInfo volumeInfo; 
        star::Light light;
        FogInfo controlInfo;
        Fog::Type type;
        TerrainShapeInfo shapeInfo;
        TerrainRenderingType terrainRenderingType;
        star::Handle hostVisibleRayDistanceBuffer;
        vk::Device vkDevice = VK_NULL_HANDLE;
        vk::Semaphore copyDone;
        uint64_t copyToHostBufferDoneValue;
        HostVisibleStorage *storage{nullptr};
    };
    std::unique_ptr<ImageWriteData> m_data = nullptr;

    void waitForCopyToDstBufferDone() const;
    double calculateAverageRayDistance() const;
};
} // namespace service::image_metric_manager