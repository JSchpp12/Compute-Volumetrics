#include "service/detail/image_metric_manager/HostVisibleStorage.hpp"

#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/core/device/system/event/ManagerRequest.hpp>

#include <star_common/helper/CastHelpers.hpp>

#include <boost/functional/hash.hpp>

namespace image_metric_manager
{

std::size_t Extent2DHash::operator()(const vk::Extent2D &e) const noexcept
{
    std::size_t seed = 0;
    boost::hash_combine(seed, e.width);
    boost::hash_combine(seed, e.height);
    return seed;
}

bool Extent2DEqual::operator()(const vk::Extent2D &a, const vk::Extent2D &b) const noexcept
{
    return a.width == b.width && a.height == b.height;
}

void HostVisibleStorage::cleanupRender()
{
    for (size_t i{0}; i < m_pools.size(); i++)
    {
        m_pools[i].rayDistBuffers = nullptr;
        m_pools[i].rayAtCutoffBuffers = nullptr;
    }

    m_resolutionToPool.clear();
}

void HostVisibleStorage::returnBuffer(const star::Handle &handle)
{
    auto &pool = getPool(handle);
    pool.rayAtCutoffBuffers->release(handle);
    pool.rayDistBuffers->release(handle);
}

bool HostVisibleStorage::contains(const vk::Extent2D &targetResolution) const
{
    return m_resolutionToPool.contains(targetResolution);
}

void HostVisibleStorage::prepRender(const star::common::FrameTracker &ft, star::common::EventBus &eb)
{
    m_copyDoneSemaphores = createTimelineSemaphore(ft, eb);
}

void HostVisibleStorage::createResourcePoolForResolution(const vk::Extent2D &resolution,
                                                         const star::common::FrameTracker &ft,
                                                         star::core::device::StarDevice &device,
                                                         star::common::EventBus &eb)
{
    std::unique_ptr<star::data_structure::dynamic::ThreadSharedObjectPool<
        star::StarBuffers::Buffer, star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 40>>
        rayDist = nullptr;

    std::unique_ptr<star::data_structure::dynamic::ThreadSharedObjectPool<
        star::StarBuffers::Buffer, star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 40>>
        rayAtCutoff = nullptr;

    {
        const size_t size = sizeof(float) * resolution.height * resolution.width;
        star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy create{
            .allocator = device.getAllocator().get(),
            .allocationName = "HRayDist",
            .allocInfo = star::Allocator::AllocationBuilder()
                             .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                             .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                                       VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT)
                             .build(),
            .createInfo = vk::BufferCreateInfo()
                              .setUsage(vk::BufferUsageFlagBits::eTransferDst)
                              .setSize(size)
                              .setSharingMode(vk::SharingMode::eExclusive),
            .instanceSize = size,
            .instanceCount = 1};

        rayDist = std::make_unique<star::data_structure::dynamic::ThreadSharedObjectPool<
            star::StarBuffers::Buffer, star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 40>>(
            create);
    }

    {
        const size_t size = sizeof(uint32_t) * resolution.height * resolution.width;
        star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy create{
            .allocator = device.getAllocator().get(),
            .allocationName = "HRayCutoff",
            .allocInfo = star::Allocator::AllocationBuilder()
                             .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                             .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                                       VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT)
                             .build(),
            .createInfo = vk::BufferCreateInfo()
                              .setUsage(vk::BufferUsageFlagBits::eTransferDst)
                              .setSize(size)
                              .setSharingMode(vk::SharingMode::eExclusive),
            .instanceSize = size,
            .instanceCount = 1};

        rayAtCutoff = std::make_unique<star::data_structure::dynamic::ThreadSharedObjectPool<
            star::StarBuffers::Buffer, star::wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 40>>(
            create);
    }

    m_pools.emplace_back(HostBuffers{std::move(rayDist), std::move(rayAtCutoff)});

    {
        uint16_t t = 0;
        star::common::helper::SafeCast(m_pools.size() - 1, t);

        star::Handle nh{.type = t};

        m_resolutionToPool.insert(std::make_pair(resolution, std::move(nh)));
    }
}

star::Handle HostVisibleStorage::getAvailableBufferToUse(const vk::Extent2D &resolutionTarget)
{
    //const auto &r = getPoolReference(resolutionTarget);
    //auto &pool = getPool(r);

    auto &pool = m_pools.front(); 
    auto rayDist = pool.rayDistBuffers->acquireBlocking();
    const auto rayCutoff = pool.rayAtCutoffBuffers->acquireBlocking();

    assert(rayDist == rayCutoff);

    //rayDist.type = r.getType();
    return rayDist;
}

void HostVisibleStorage::getRayDistanceBuffers(const star::Handle &handle,
                                               const star::StarBuffers::Buffer **rayDistBuffer,
                                               const star::StarBuffers::Buffer **rayAtCutoffDistBuffer) const
{
    auto &pool = getPool(handle);

    *rayDistBuffer = &pool.rayDistBuffers->get(handle);
    *rayAtCutoffDistBuffer = &pool.rayAtCutoffBuffers->get(handle);
}

std::vector<star::Handle> HostVisibleStorage::createTimelineSemaphore(const star::common::FrameTracker &ft,
                                                                      star::common::EventBus &eb) const
{
    const size_t num = static_cast<size_t>(ft.getSetup().getNumFramesInFlight());

    auto handles = std::vector<star::Handle>(num);
    for (size_t i{0}; i < handles.size(); i++)
    {
        void *r = nullptr;
        eb.emit(star::core::device::system::event::ManagerRequest(
            star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                star::core::device::manager::GetSemaphoreEventTypeName),
            star::core::device::manager::SemaphoreRequest{true}, handles[i], &r));

        if (r == nullptr)
        {
            STAR_THROW("Unable to create new semaphore");
        }
    }

    return handles;
}

const star::Handle &HostVisibleStorage::getPoolReference(const vk::Extent2D &targetResolution) const
{
    assert(m_resolutionToPool.contains(targetResolution));

    return m_resolutionToPool.at(targetResolution);
}

HostVisibleStorage::HostBuffers &HostVisibleStorage::getPool(const star::Handle &handle)
{
    const size_t index = static_cast<size_t>(handle.getType()); 
    assert(handle.getType() < m_pools.size());

    return m_pools[static_cast<size_t>(handle.getType())];
}

const HostVisibleStorage::HostBuffers &HostVisibleStorage::getPool(const star::Handle &handle) const
{
    const size_t index = static_cast<size_t>(handle.getType());
    assert(index < m_pools.size());

    return m_pools[index];
}
} // namespace image_metric_manager