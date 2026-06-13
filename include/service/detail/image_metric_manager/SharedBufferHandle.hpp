#pragma once

#include "service/detail/image_metric_manager/CopyDstResources.hpp"
#include "service/detail/image_metric_manager/HostVisibleStorage.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>

namespace service::image_metric_manager
{
class SharedBufferHandle
{
  public:
    SharedBufferHandle(HostVisibleStorage *pool, star::Handle handle, CopyDstResources resources,
                       vk::Extent2D resolution, vk::Device device, vk::Semaphore copyDone, uint64_t copyDoneValue);

    SharedBufferHandle(const SharedBufferHandle &) = delete;
    SharedBufferHandle &operator=(const SharedBufferHandle &) = delete;

    ~SharedBufferHandle();

    void waitForCopyToDstBufferDone() const;

    void ensureMapped();

    const float *getMappedRayDistanceData() const;
    const uint32_t *getMappedRayAtCutoffDistData() const;
    size_t getRayDistanceElementCount() const;
    size_t getRayAtCutoffDistElementCount() const;

    const CopyDstResources &getResources() const; 
    const star::StarBuffers::Buffer &getRayDistanceBuffer() const;
    const star::StarBuffers::Buffer &getRayAtCutoffDistBuffer() const;
    vk::Extent2D getResolution() const;

  private:
    HostVisibleStorage *m_pool;
    star::Handle m_handle;
    CopyDstResources m_resources;
    vk::Extent2D m_resolution;
    vk::Device m_device;
    vk::Semaphore m_copyDone;
    uint64_t m_copyDoneValue;

    void *m_mappedRayDistanceData = nullptr;
    void *m_mappedRayAtCutoffDistData = nullptr;
    bool m_mapped = false;
};
} // namespace service::image_metric_manager