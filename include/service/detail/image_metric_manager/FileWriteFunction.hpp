#pragma once

#include "FogInfo.hpp"
#include "FogType.hpp"
#include "TerrainRenderingType.hpp"
#include "TerrainShapeInfo.hpp"
#include "service/detail/image_metric_manager/HostVisibleStorage.hpp"

#include <starlight/common/entities/Light.hpp>

#include <vulkan/vulkan.hpp>

namespace service::image_metric_manager
{
class FileWriteFunction
{
  public:
    FileWriteFunction() = default;
    FileWriteFunction(star::Light light, FogInfo controlInfo, glm::vec3 camPosition, glm::vec3 cameraLookDir,
                      star::Handle buffer, vk::Device vkDevice, vk::Semaphore done, uint64_t copyToHostBufferDoneValue,
                      Fog::Type type, HostVisibleStorage *storage, std::string terrainName,
                      TerrainShapeInfo terrainShapeInfo, TerrainRenderingType terrainRenderingType);

    void write(const std::filesystem::path &path) const;

    int operator()(const std::filesystem::path &filePath);

  private:
    struct ImageWriteData
    {
        star::Light light;
        TerrainShapeInfo shapeInfo;
        std::string terrainName;
        FogInfo controlInfo;
        glm::vec3 camPosition;
        glm::vec3 camLookDir;
        star::Handle hostVisibleRayDistanceBuffer;
        vk::Device vkDevice = VK_NULL_HANDLE;
        vk::Semaphore copyDone;
        uint64_t copyToHostBufferDoneValue;
        Fog::Type type;
        TerrainRenderingType terrainRenderingType;
        HostVisibleStorage *storage{nullptr};
    };
    std::unique_ptr<ImageWriteData> m_data = nullptr;

    void waitForCopyToDstBufferDone() const;
    double calculateAverageRayDistance() const;
};
} // namespace service::image_metric_manager