#include "service/detail/image_metric_manager/FileWriteFunction.hpp"

#include <boost/filesystem/path.hpp>

#include "service/detail/image_metric_manager/ImageMetrics.hpp"

namespace image_metric_manager
{

// WARNING: may result in slight inconsistency or error in result
static double Mean(std::span<const float> &span)
{
    if (span.empty())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double sum = std::reduce(std::execution::unseq, span.begin(), span.end(), 0.0);
    return sum / static_cast<double>(span.size());
}

FileWriteFunction::FileWriteFunction(star::Handle buffer, vk::Device vkDevice, vk::Semaphore done,
                                     uint32_t copyToHostBufferDoneValue, HostVisibleStorage *storage)
    : m_hostVisibleRayDistanceBuffer(std::move(buffer)), m_vkDevice(std::move(vkDevice)), m_copyDone(std::move(done)),
      m_copyToHostBufferDoneValue(copyToHostBufferDoneValue), m_storage(storage)
{
}

void FileWriteFunction::write(const std::string &path) const
{
    assert(m_storage != nullptr && "Host storage should have been provided");

    auto fPath = boost::filesystem::path(path);
    fPath.replace_extension(".json");

    {
        const std::string msg = "Beginning file write: " + fPath.string();
        star::core::logging::info(msg);
    }

    waitForCopyToDstBufferDone();

    double mean = calculateAverageRayDistance();
    m_storage->returnBuffer(m_hostVisibleRayDistanceBuffer);

    std::ofstream out(fPath.string(), std::ofstream::binary);
    const auto data = ImageMetrics(fPath.filename().string(), mean).toJsonDump();
    out << data;

    star::core::logging::info("Done");
}

void FileWriteFunction::waitForCopyToDstBufferDone() const
{
    assert(m_vkDevice != VK_NULL_HANDLE);

    uint64_t value = uint64_t(m_copyToHostBufferDoneValue);
    vk::Result waitResult =
        m_vkDevice.waitSemaphores(vk::SemaphoreWaitInfo().setValues(value).setSemaphores(m_copyDone), UINT64_MAX);

    if (waitResult != vk::Result::eSuccess)
    {
        STAR_THROW("Failed to wait for semaphores");
    }
}

double FileWriteFunction::calculateAverageRayDistance() const
{
    assert(m_storage != nullptr);

    const star::StarBuffers::Buffer *rayDistance = nullptr;
    const star::StarBuffers::Buffer *rayAtCutoff = nullptr;
    m_storage->getRayDistanceBuffers(m_hostVisibleRayDistanceBuffer, &rayDistance, &rayAtCutoff);

    assert(rayDistance != nullptr && rayAtCutoff != nullptr && "Failed to get buffers");
    uint32_t nAtMax = getNumRaysAtMaxDistance(*rayAtCutoff);
    double mean = 0.0;
    {
        void *d = nullptr;
        rayDistance->map(&d);

        auto *data = static_cast<const float *>(d);
        const size_t n = static_cast<size_t>(rayDistance->getBufferSize() / sizeof(float));
        std::span<const float> span{data, n};

        const size_t numRaysToConsider = n - static_cast<size_t>(nAtMax);
        const float s = sum(span);

        rayDistance->unmap();
        mean = (double)s / (double)numRaysToConsider;
    }

    return mean;
}

uint32_t FileWriteFunction::getNumRaysAtMaxDistance(const star::StarBuffers::Buffer &computeRayAtMaxBuffer) const
{
    uint32_t nAtMax = 0;
    void *d = nullptr;
    computeRayAtMaxBuffer.map(&d);

    auto *data = static_cast<const uint32_t *>(d);
    const size_t n = static_cast<size_t>(computeRayAtMaxBuffer.getBufferSize() / sizeof(uint32_t));
    std::span<const uint32_t> span{data, n};

    nAtMax = sum(span);
    computeRayAtMaxBuffer.unmap();

    return nAtMax;
}
} // namespace image_metric_manager