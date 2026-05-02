#include "service/detail/image_metric_manager/FileWriteFunction.hpp"

#include "service/detail/image_metric_manager/ImageMetrics.hpp"
#include <boost/filesystem/path.hpp>
#include <starlight/common/entities/Light_json.hpp>

#include <execution>
#include <numeric>
#include <span>

namespace service::image_metric_manager
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

FileWriteFunction::FileWriteFunction(star::Light light, FogInfo controlInfo, glm::vec3 camPosition,
                                     glm::vec3 camLookDir, star::Handle buffer, vk::Device vkDevice, vk::Semaphore done,
                                     uint64_t copyToHostBufferDoneValue, Fog::Type type, HostVisibleStorage *storage,
                                     TerrainShapeInfo terrainShapeInfo)
    : m_data(std::make_unique<ImageWriteData>(std::move(light), std::move(terrainShapeInfo), std::move(controlInfo),
                                              camPosition, camLookDir, buffer, vkDevice, done,
                                              copyToHostBufferDoneValue, type, storage))
{
}

void FileWriteFunction::write(const std::filesystem::path &path) const
{
    const auto sourcePath = boost::filesystem::path(path);
    const auto finalPath = boost::filesystem::path(path).replace_extension(".json");

    waitForCopyToDstBufferDone();

    double mean = calculateAverageRayDistance();
    m_data->storage->returnBuffer(m_data->hostVisibleRayDistanceBuffer);

    std::ofstream out(finalPath.string(), std::ofstream::binary);
    const auto data = ImageMetrics(m_data->light, m_data->controlInfo, m_data->camPosition, m_data->camLookDir,
                                   sourcePath.filename().string(), mean, m_data->type, m_data->shapeInfo)
                          .toJsonDump();
    out << data;
}

void FileWriteFunction::waitForCopyToDstBufferDone() const
{
    assert(m_data->vkDevice != VK_NULL_HANDLE);

    try
    {
        vk::Result waitResult = m_data->vkDevice.waitSemaphores(
            vk::SemaphoreWaitInfo().setValues(m_data->copyToHostBufferDoneValue).setSemaphores(m_data->copyDone),
            UINT64_MAX);

        if (waitResult != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to wait for semaphores");
        }
    }
    catch (const vk::DeviceLostError &e)
    {
        std::ostringstream oss;
        oss << "Vulkan error encountered while submitting queue. Terminating. " << e.what();
        STAR_THROW(oss.str());
    }
}

// cutoff factory is some amount (%) when it becomes "foggy" used as mixing value in shader
static double CalcVisDistanceExponential(const FogInfo &info, const float &cutoffValue)
{
    return -std::log(1.0 - cutoffValue) / (double)info.expFogInfo.density;
}

static double CalcVisDistanceLinear(const FogInfo &info)
{
    return info.linearInfo.farDist;
}

static uint32_t Sum(std::span<const uint32_t> &span)
{
    if (span.empty())
    {
        return 0;
    }

    return std::reduce(std::execution::unseq, span.begin(), span.end(), 0.0);
}

static float Sum(std::span<const float> &span)
{
    if (span.empty())
    {
        return 0;
    }

    return std::reduce(std::execution::unseq, span.begin(), span.end(), 0.0);
}

static uint32_t GetNumRaysAtMaxDistance(const star::StarBuffers::Buffer &computeRayAtMaxBuffer)
{
    void *d = nullptr;
    auto result = computeRayAtMaxBuffer.invalidate();
    if (result != vk::Result::eSuccess)
    {
        STAR_THROW("Failed to invalidate memory");
    }
    computeRayAtMaxBuffer.map(&d);

    auto *data = static_cast<const uint32_t *>(d);
    const size_t n = static_cast<size_t>(computeRayAtMaxBuffer.getBufferSize() / sizeof(uint32_t));
    std::span<const uint32_t> span{data, n};
    uint32_t nAtMax = std::reduce(std::execution::unseq, span.begin(), span.end(), 0u, std::plus<uint32_t>());
    computeRayAtMaxBuffer.unmap();

    return nAtMax;
}

static double CalcVisDistanceFromRayBuffers(const star::StarBuffers::Buffer &rayDistance,
                                            const star::StarBuffers::Buffer &rayAtCutoff)
{
    uint32_t nAtMax = GetNumRaysAtMaxDistance(rayAtCutoff);
    double mean = 0.0;
    {
        void *d = nullptr;
        rayDistance.map(&d);

        auto *data = static_cast<const float *>(d);
        const size_t n = static_cast<size_t>(rayDistance.getBufferSize() / sizeof(float));
        std::span<const float> span{data, n};

        const size_t numRaysToConsider = n - static_cast<size_t>(nAtMax);
        const float s = Sum(span);

        rayDistance.unmap();
        mean = (double)s / (double)numRaysToConsider;
    }

    return mean;
}

double FileWriteFunction::calculateAverageRayDistance() const
{
    assert(m_data->storage != nullptr);

    const star::StarBuffers::Buffer *rayDistance = nullptr;
    const star::StarBuffers::Buffer *rayAtCutoff = nullptr;
    m_data->storage->getRayDistanceBuffers(m_data->hostVisibleRayDistanceBuffer, &rayDistance, &rayAtCutoff);
    assert(rayDistance != nullptr && rayAtCutoff != nullptr && "Failed to get buffers");

    double distance = 0.0;
    switch (m_data->type)
    {
    case (Fog::Type::sExponential):
        distance = CalcVisDistanceExponential(m_data->controlInfo, 0.98f);
        break;
    case (Fog::Type::sLinear):
        distance = CalcVisDistanceLinear(m_data->controlInfo);
        break;
    default:
        distance = CalcVisDistanceFromRayBuffers(*rayDistance, *rayAtCutoff);
        break;
    }

    return distance;
}

int FileWriteFunction::operator()(const std::filesystem::path &filePath)
{
    write(filePath);

    return 0;
}

} // namespace service::image_metric_manager