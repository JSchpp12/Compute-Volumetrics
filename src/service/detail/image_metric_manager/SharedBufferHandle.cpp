#include "service/detail/image_metric_manager/SharedBufferHandle.hpp"

#include <starlight/core/Exceptions.hpp>

#include <sstream>

namespace service::image_metric_manager
{
SharedBufferHandle::SharedBufferHandle(HostVisibleStorage *pool, star::Handle handle, CopyDstResources resources,
                                       vk::Extent2D resolution, vk::Device device, vk::Semaphore copyDone,
                                       uint64_t copyDoneValue)
    : m_resources(std::move(resources)), m_copyDoneValue(std::move(copyDoneValue)), m_resolution(std::move(resolution)),
      m_handle(std::move(handle)), m_pool(pool), m_device(std::move(device)), m_copyDone(std::move(copyDone))
{
}

SharedBufferHandle::~SharedBufferHandle()
{
    {
        std::lock_guard lock(m_mappingMutex);
        if (m_mapRefCount > 0)
        {
            m_resources.rayDistanceBuffer->unmap();
            m_resources.rayAtCutoffDistBuffer->unmap();
            m_mappedRayDistanceData = nullptr;
            m_mappedRayAtCutoffDistData = nullptr;
            m_mapRefCount = 0;
        }
    }

    if (m_pool != nullptr)
    {
        m_pool->returnResource(m_handle);
    }
}

void SharedBufferHandle::waitForCopyToDstBufferDone() const
{
    assert(m_device != VK_NULL_HANDLE);

    try
    {
        vk::Result waitResult = m_device.waitSemaphores(
            vk::SemaphoreWaitInfo().setValues(m_copyDoneValue).setSemaphores(m_copyDone), UINT64_MAX);

        if (waitResult != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to wait for semaphores");
        }
    }
    catch (const vk::DeviceLostError &e)
    {
        std::ostringstream oss;
        oss << "Vulkan error encountered while waiting for copy to host buffer. Terminating. " << e.what();
        STAR_THROW(oss.str());
    }
}

void SharedBufferHandle::ensureMapped()
{
    std::lock_guard lock(m_mappingMutex);

    ++m_mapRefCount;
    if (m_mapRefCount > 1)
        return;

    assert(m_resources.rayDistanceBuffer != nullptr);
    assert(m_resources.rayAtCutoffDistBuffer != nullptr);

    auto distanceResult = m_resources.rayDistanceBuffer->invalidate();
    if (distanceResult != vk::Result::eSuccess)
    {
        STAR_THROW("Failed to invalidate ray distance buffer");
    }

    auto maskResult = m_resources.rayAtCutoffDistBuffer->invalidate();
    if (maskResult != vk::Result::eSuccess)
    {
        STAR_THROW("Failed to invalidate ray at cutoff distance buffer");
    }

    m_resources.rayDistanceBuffer->map(&m_mappedRayDistanceData);
    m_resources.rayAtCutoffDistBuffer->map(&m_mappedRayAtCutoffDistData);

    if (!m_mappedRayDistanceData)
    {
        STAR_THROW("Failed to map ray distance buffer");
    }
    if (!m_mappedRayAtCutoffDistData)
    {
        STAR_THROW("Failed to map ray at cutoff distance buffer");
    }
}

void SharedBufferHandle::ensureUnmapped()
{
    std::lock_guard lock(m_mappingMutex);

    if (m_mapRefCount == 0)
        return;

    --m_mapRefCount;
    if (m_mapRefCount == 0)
    {
        m_resources.rayDistanceBuffer->unmap();
        m_resources.rayAtCutoffDistBuffer->unmap();

        m_mappedRayDistanceData = nullptr;
        m_mappedRayAtCutoffDistData = nullptr;
    }
}

const float *SharedBufferHandle::getMappedRayDistanceData() const
{
    assert(m_mappedRayDistanceData != nullptr);
    return static_cast<const float *>(m_mappedRayDistanceData);
}

const uint32_t *SharedBufferHandle::getMappedRayAtCutoffDistData() const
{
    assert(m_mappedRayAtCutoffDistData != nullptr);
    return static_cast<const uint32_t *>(m_mappedRayAtCutoffDistData);
}

size_t SharedBufferHandle::getRayDistanceElementCount() const
{
    assert(m_resources.rayDistanceBuffer != nullptr);
    return static_cast<size_t>(m_resources.rayDistanceBuffer->getBufferSize() / sizeof(float));
}

size_t SharedBufferHandle::getRayAtCutoffDistElementCount() const
{
    assert(m_resources.rayAtCutoffDistBuffer != nullptr);
    return static_cast<size_t>(m_resources.rayAtCutoffDistBuffer->getBufferSize() / sizeof(uint32_t));
}

const vk::Extent2D &SharedBufferHandle::getResolution() const
{
    return m_resolution;
}

vk::Extent3D SharedBufferHandle::getImageExtent() const
{
    return vk::Extent3D().setWidth(m_resolution.width).setHeight(m_resolution.height).setDepth(1);
}
} // namespace service::image_metric_manager