#pragma once

#include "FogControlInfo.hpp"
#include "FogType.hpp"
#include "service/detail/image_metric_manager/HostVisibleStorage.hpp"

#include <starlight/core/CommandSubmitter.hpp>
#include <starlight/wrappers/graphics/StarBuffers/Buffer.hpp>

#include <vulkan/vulkan.hpp>

#include <execution>
#include <numeric>
#include <span>

namespace image_metric_manager
{
class FileWriteFunction
{
  public:
    FileWriteFunction() = default;
    FileWriteFunction(FogInfo controlInfo, glm::vec3 camPosition, star::Handle buffer, vk::Device vkDevice, vk::Semaphore done,
                      uint64_t copyToHostBufferDoneValue, Fog::Type type, HostVisibleStorage *storage);

    void write(const std::string &path) const;

    int operator()(const std::string &filePath);

  private:
    struct ImageWriteData
    {
        FogInfo controlInfo;
        glm::vec3 camPosition;
        star::Handle hostVisibleRayDistanceBuffer;
        vk::Device vkDevice = VK_NULL_HANDLE;
        vk::Semaphore copyDone;
        uint64_t copyToHostBufferDoneValue;
        Fog::Type type;
        HostVisibleStorage *storage = nullptr;
    };
    std::unique_ptr<ImageWriteData> m_data = nullptr; 

    void waitForCopyToDstBufferDone() const;
    double calculateAverageRayDistance() const;
};
} // namespace image_metric_manager