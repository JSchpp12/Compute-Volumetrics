#include "service/detail/image_metric_manager/FileWriteFunction.hpp"

#include "service/detail/image_metric_manager/ImageMetrics.hpp"

#include <starlight/common/entities/Light_json.hpp>
#include <starlight/core/helper/star_object/ObjectHelpers.hpp>

#include <boost/filesystem/path.hpp>

#include <algorithm>
#include <execution>
#include <limits>
#include <numeric>
#include <span>

namespace service::image_metric_manager
{

FileWriteFunction::FileWriteFunction(const star::StarCamera &camera, const Volume &volume, star::Light light,
                                     star::Handle buffer, vk::Device vkDevice, vk::Semaphore done,
                                     uint64_t copyToHostBufferDoneValue, HostVisibleStorage *storage,
                                     std::string terrainName, TerrainShapeInfo terrainShapeInfo,
                                     TerrainRenderingType terrainRenderingType, std::string volumeName)
    : m_data(std::make_unique<ImageWriteData>(
          std::move(terrainName), std::move(volumeName),
          ImageWriteData::CameraInfo{camera.getPosition(), camera.getForwardVector()},
          VolumeInfo{.position = volume.getInstance().getPosition(),
                     .rotation =
                         star::core::helper::star_object::ExtractRotationDegrees(volume.getInstance().getRotationMat()),
                     .scale = volume.getInstance().getScale()},
          std::move(light), volume.getRenderer().getFogInfo(), volume.getRenderer().getFogType(),
          std::move(terrainShapeInfo), terrainRenderingType, buffer, vkDevice, done, copyToHostBufferDoneValue,
          storage))
{
}

void FileWriteFunction::write(const std::filesystem::path &path) const
{
    const auto sourcePath = boost::filesystem::path(path);
    const auto finalPath = boost::filesystem::path(path).replace_extension(".json");

    waitForCopyToDstBufferDone();

    const RayDistanceMetrics distanceMetrics = calculateDistanceMetrics();
    m_data->storage->returnBuffer(m_data->hostVisibleRayDistanceBuffer);

    std::ofstream out(finalPath.string(), std::ofstream::binary);
    const auto data =
        ImageMetrics(m_data->light, m_data->volumeInfo, m_data->controlInfo, m_data->cameraInfo.position,
                     m_data->cameraInfo.lookDir, sourcePath.filename().string(), distanceMetrics,
                     m_data->terrainName, m_data->volumeName, m_data->type, m_data->shapeInfo,
                     m_data->terrainRenderingType)
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

static RayDistanceMetrics CalcVisDistanceFromRayBuffers(const star::StarBuffers::Buffer &rayDistance,
                                                       const star::StarBuffers::Buffer &validRayMask)
{
    auto distanceResult = rayDistance.invalidate();
    if (distanceResult != vk::Result::eSuccess)
    {
        STAR_THROW("Failed to invalidate ray distance memory");
    }

    auto maskResult = validRayMask.invalidate();
    if (maskResult != vk::Result::eSuccess)
    {
        STAR_THROW("Failed to invalidate valid ray mask memory");
    }

    void *distanceData = nullptr;
    void *maskData = nullptr;
    rayDistance.map(&distanceData);
    validRayMask.map(&maskData);

    const auto *distances = static_cast<const float *>(distanceData);
    const auto *validMask = static_cast<const float *>(maskData);
    const size_t distanceCount = static_cast<size_t>(rayDistance.getBufferSize() / sizeof(float));
    const size_t maskCount = static_cast<size_t>(validRayMask.getBufferSize() / sizeof(float));

    if (distanceCount != maskCount)
    {
        validRayMask.unmap();
        rayDistance.unmap();
        STAR_THROW("Ray distance and valid ray mask buffers have different element counts");
    }

    std::span<const float> distanceSpan{distances, distanceCount};
    std::span<const float> validMaskSpan{validMask, maskCount};

    RayDistanceMetrics result{{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()},
                              {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()},
                              0};

    if (!distanceSpan.empty())
    {
        result.rayCount = distanceSpan.size();

        const double includingInvalidSum =
            std::transform_reduce(std::execution::unseq, distanceSpan.begin(), distanceSpan.end(),
                                  validMaskSpan.begin(), 0.0, std::plus<double>(),
                                  [](const float distance, const float) { return static_cast<double>(distance); });

        const double excludingInvalidSum =
            std::transform_reduce(std::execution::unseq, distanceSpan.begin(), distanceSpan.end(),
                                  validMaskSpan.begin(), 0.0, std::plus<double>(),
                                  [](const float distance, const float valid) {
                                      return static_cast<double>(distance) * static_cast<double>(valid);
                                  });

        const double validRayCount =
            std::transform_reduce(std::execution::unseq, distanceSpan.begin(), distanceSpan.end(),
                                  validMaskSpan.begin(), 0.0, std::plus<double>(),
                                  [](const float, const float valid) { return static_cast<double>(valid); });

        const double includingInvalidMin = std::transform_reduce(
            std::execution::unseq, distanceSpan.begin(), distanceSpan.end(), validMaskSpan.begin(),
            std::numeric_limits<double>::infinity(),
            [](const double lhs, const double rhs) { return std::min(lhs, rhs); },
            [](const float distance, const float) { return static_cast<double>(distance); });

        const double excludingInvalidMin = std::transform_reduce(
            std::execution::unseq, distanceSpan.begin(), distanceSpan.end(), validMaskSpan.begin(),
            std::numeric_limits<double>::infinity(),
            [](const double lhs, const double rhs) { return std::min(lhs, rhs); },
            [](const float distance, const float valid) {
                return valid > 0.0f ? static_cast<double>(distance) : std::numeric_limits<double>::infinity();
            });

        result.includingInvalidRays.average = includingInvalidSum / static_cast<double>(distanceSpan.size());
        result.excludingInvalidRays.average =
            validRayCount > 0.0 ? excludingInvalidSum / validRayCount : std::numeric_limits<double>::quiet_NaN();
        result.includingInvalidRays.minimum = includingInvalidMin;
        result.excludingInvalidRays.minimum =
            validRayCount > 0.0 ? excludingInvalidMin : std::numeric_limits<double>::quiet_NaN();
    }

    validRayMask.unmap();
    rayDistance.unmap();

    return result;
}

RayDistanceMetrics FileWriteFunction::calculateDistanceMetrics() const
{
    assert(m_data->storage != nullptr);

    const star::StarBuffers::Buffer *rayDistance = nullptr;
    const star::StarBuffers::Buffer *rayAtCutoff = nullptr;
    m_data->storage->getRayDistanceBuffers(m_data->hostVisibleRayDistanceBuffer, &rayDistance, &rayAtCutoff);
    assert(rayDistance != nullptr && rayAtCutoff != nullptr && "Failed to get buffers");

    RayDistanceMetrics distance;
    switch (m_data->type)
    {
    case (Fog::Type::sExponential):
    {
        const double val = CalcVisDistanceExponential(m_data->controlInfo, 0.98f);
        distance.includingInvalidRays.average = val;
        distance.excludingInvalidRays.average = val;
        distance.includingInvalidRays.minimum = val;
        distance.excludingInvalidRays.minimum = val;
        break;
    }
    case (Fog::Type::sLinear):
    {
        const double val = CalcVisDistanceLinear(m_data->controlInfo);
        distance.includingInvalidRays.average = val;
        distance.excludingInvalidRays.average = val;
        distance.includingInvalidRays.minimum = val;
        distance.excludingInvalidRays.minimum = val;
        break;
    }
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
