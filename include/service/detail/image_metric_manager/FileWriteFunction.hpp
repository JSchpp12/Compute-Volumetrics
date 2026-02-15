#pragma once

#include "service/detail/image_metric_manager/HostVisibleStorage.hpp"

#include <starlight/wrappers/graphics/StarBuffers/Buffer.hpp>
#include <starlight/core/CommandSubmitter.hpp>

#include <vulkan/vulkan.hpp>

#include <execution>
#include <numeric>
#include <span>

namespace image_metric_manager
{
class FileWriteFunction
{
  public:
    FileWriteFunction(star::Handle buffer, vk::Device vkDevice, vk::Semaphore done, uint64_t copyToHostBufferDoneValue,
                      HostVisibleStorage *storage);

    void write(const std::string &path) const;

  private:
    star::Handle m_hostVisibleRayDistanceBuffer;
    vk::Device m_vkDevice = VK_NULL_HANDLE;
    vk::Semaphore m_copyDone;
    uint64_t m_copyToHostBufferDoneValue;
    HostVisibleStorage *m_storage = nullptr;

    void waitForCopyToDstBufferDone() const;
    double calculateAverageRayDistance() const;
    uint32_t getNumRaysAtMaxDistance(const star::StarBuffers::Buffer &computeRayAtMaxBuffer) const;

    template <typename T> T sum(std::span<const T> &span) const
    {
        if (span.empty())
        {
            return 0;
        }

        return std::reduce(std::execution::unseq, span.begin(), span.end(), 0.0);
    }
};
} // namespace image_metric_manager