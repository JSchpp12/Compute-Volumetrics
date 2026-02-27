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
    FileWriteFunction(FogInfo controlInfo, star::Handle buffer, vk::Device vkDevice, vk::Semaphore done,
                      uint64_t copyToHostBufferDoneValue, Fog::Type type, HostVisibleStorage *storage);

    void write(const std::string &path) const;

    int operator()(const std::string &filePath); 

  private:
    FogInfo m_controlInfo;
    star::Handle m_hostVisibleRayDistanceBuffer;
    vk::Device m_vkDevice = VK_NULL_HANDLE;
    vk::Semaphore m_copyDone;
    uint64_t m_copyToHostBufferDoneValue;
    Fog::Type m_type;
    HostVisibleStorage *m_storage = nullptr;

    void waitForCopyToDstBufferDone() const;
    double calculateAverageRayDistance() const;
};
} // namespace image_metric_manager