#pragma once

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/data_structure/dynamic/ThreadSharedObjectPool.hpp>
#include <starlight/wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp>

#include <star_common/EventBus.hpp>

#include <absl/container/flat_hash_map.h>

namespace image_metric_manager
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

    star::Handle getAvailableBufferToUse(const vk::Extent2D &targetResolution);

    void returnBuffer(const star::Handle &handle);

    void getRayDistanceBuffers(const star::Handle &handle, const star::StarBuffers::Buffer **rayDistBuffer,
                               const star::StarBuffers::Buffer **rayAtCutoffDistBuffer) const;

    bool contains(const vk::Extent2D &targetResolution) const;

    void createResourcePoolForResolution(const vk::Extent2D &resolution, const star::common::FrameTracker &ft,
                                         star::core::device::StarDevice &device, star::common::EventBus &eb);

    const std::vector<star::Handle> &getCopyDoneSemaphores() const
    {
        return m_copyDoneSemaphores;
    }

  private:
    struct HostBuffers
    {
        std::unique_ptr<star::data_structure::dynamic::ThreadSharedObjectPool<
            star::StarBuffers::Buffer, star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 25>>
            rayDistBuffers;
        std::unique_ptr<star::data_structure::dynamic::ThreadSharedObjectPool<
            star::StarBuffers::Buffer, star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 25>>
            rayAtCutoffBuffers;
    };
    std::vector<HostBuffers> m_pools;
    absl::flat_hash_map<vk::Extent2D, star::Handle, Extent2DHash, Extent2DEqual> m_resolutionToPool;

    std::vector<star::Handle> m_copyDoneSemaphores;

    std::vector<star::Handle> createTimelineSemaphore(const star::common::FrameTracker &ft,
                                                      star::common::EventBus &eb) const;

    const star::Handle &getPoolReference(const vk::Extent2D &targetResolution) const;

    HostBuffers &getPool(const star::Handle &handle);

    const HostBuffers &getPool(const star::Handle &handle) const;
};
} // namespace image_metric_manager