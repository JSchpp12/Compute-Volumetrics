#pragma once

#include "service/detail/image_metric_manager/CopyDstResources.hpp"
#include "service/detail/image_metric_manager/HostVisibleStorage.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <memory>
#include <mutex>

namespace service::image_metric_manager
{
class SharedBufferHandle
{
  public:
    SharedBufferHandle(HostVisibleStorage *pool, star::Handle handle, CopyDstResources resources,
                       vk::Extent2D resolution, vk::Device device, vk::Semaphore copyDone, uint64_t copyDoneValue);
    SharedBufferHandle(const SharedBufferHandle &) = delete;
    SharedBufferHandle &operator=(const SharedBufferHandle &) = delete;
    SharedBufferHandle(SharedBufferHandle &&) = delete;
    SharedBufferHandle &operator=(SharedBufferHandle &&) = delete;
    ~SharedBufferHandle();

    void waitForCopyToDstBufferDone() const;
    void ensureMapped();
    void ensureUnmapped();
    const float *getMappedRayDistanceData() const;
    const uint32_t *getMappedRayAtCutoffDistData() const;
    size_t getRayDistanceElementCount() const;
    size_t getRayAtCutoffDistElementCount() const;
    const vk::Extent2D &getResolution() const;
    vk::Extent3D getImageExtent() const;

  private:
    mutable std::mutex m_mappingMutex;
    CopyDstResources m_resources;
    uint64_t m_copyDoneValue;
    vk::Extent2D m_resolution;
    star::Handle m_handle;
    HostVisibleStorage *m_pool;
    vk::Device m_device;
    vk::Semaphore m_copyDone;
    void *m_mappedRayDistanceData{nullptr};
    void *m_mappedRayAtCutoffDistData{nullptr};
    uint8_t m_mapRefCount{0};
};
} // namespace service::image_metric_manager