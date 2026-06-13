#pragma once

#include "service/detail/image_metric_manager/CopyDstResources.hpp"

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/data_structure/dynamic/ThreadSharedObjectPool.hpp>
#include <starlight/wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp>

#include <star_common/EventBus.hpp>

#include <absl/container/flat_hash_map.h>

namespace service::image_metric_manager
{
struct Extent2DHash
{
    std::size_t operator()(const vk::Extent2D &e) const noexcept;
};
struct Extent2DEqual
{
    bool operator()(const vk::Extent2D &a, const vk::Extent2D &b) const noexcept;
};
class HostVisibleStorage
{
  public:
    void prepRender(const star::common::FrameTracker &ft, star::common::EventBus &eb);

    void cleanupRender();

    star::Handle getAvailableResource(const vk::Extent2D &targetResolution);

    void returnResource(const star::Handle &handle);

    CopyDstResources getResource(const star::Handle &handle) const;

    bool contains(const vk::Extent2D &targetResolution) const;

    void createResourcePoolForResolution(const vk::Extent2D &resolution, const star::common::FrameTracker &ft,
                                         star::core::device::StarDevice &device, star::common::EventBus &eb);

    const std::vector<star::Handle> &getCopyDoneSemaphores() const
    {
        return m_copyDoneSemaphores;
    }

  private:
    struct HostData
    {
        std::unique_ptr<star::data_structure::dynamic::ThreadSharedObjectPool<
            star::StarBuffers::Buffer, star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 10>>
            rayDistBuffers;
        std::unique_ptr<star::data_structure::dynamic::ThreadSharedObjectPool<
            star::StarBuffers::Buffer, star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 10>>
            rayAtCutoffBuffers;
    };

    std::vector<HostData> m_pools;
    absl::flat_hash_map<vk::Extent2D, star::Handle, Extent2DHash, Extent2DEqual> m_resolutionToPool;

    std::vector<star::Handle> m_copyDoneSemaphores;

    std::vector<star::Handle> createTimelineSemaphore(const star::common::FrameTracker &ft,
                                                      star::common::EventBus &eb) const;

    const star::Handle &getPoolReference(const vk::Extent2D &targetResolution) const;

    HostData &getPoolData(const star::Handle &handle);

    const HostData &getPoolData(const star::Handle &handle) const;
};
} // namespace service::image_metric_manager