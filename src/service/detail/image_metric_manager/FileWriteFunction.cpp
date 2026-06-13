#include "service/detail/image_metric_manager/FileWriteFunction.hpp"

#include "service/detail/image_metric_manager/ImageMetrics.hpp"

#include <starlight/common/entities/Light_json.hpp>
#include <starlight/core/helper/star_object/ObjectHelpers.hpp>
#include <starlight/job/tasks/actions/WriteTiffImageAction.hpp>

#include <boost/filesystem/path.hpp>

#include <algorithm>
#include <cstdint>
#include <execution>
#include <limits>
#include <numeric>
#include <span>

namespace service::image_metric_manager
{

FileWriteFunction::FileWriteFunction(vk::Extent2D screenResolution, const star::StarCamera &camera,
                                     const Volume &volume, star::Light light, star::Handle buffer, vk::Device vkDevice,
                                     vk::Semaphore done, uint64_t copyToHostBufferDoneValue,
                                     HostVisibleStorage *storage, std::string terrainName,
                                     TerrainShapeInfo terrainShapeInfo, TerrainRenderingType terrainRenderingType,
                                     std::string volumeName)
    : m_data(std::make_unique<ImageWriteData>(
          std::move(terrainName), std::move(volumeName), std::move(screenResolution),
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

static void WriteDistanceMask(std::string maskPath, const star::StarBuffers::Buffer &rayDistance,
                              vk::Extent3D imageExtent)
{
    star::job::tasks::actions::WriteTiffImageAction action{.imageExtent = std::move(imageExtent),
                                                           .imageFormat = vk::Format::eR32Sfloat,
                                                           .path = std::move(maskPath),
                                                           .buffer = rayDistance};
    action();
}

static std::string MakeMaskPath(const std::filesystem::path &path)
{
    const std::string tifName = path.stem().string() + std::string("_mask.tif");
    const auto tifPath = path.parent_path() / std::filesystem::path(tifName);
    return tifPath.string();
}

void FileWriteFunction::write(const std::filesystem::path &path) const
{
    const auto sourcePath = std::filesystem::path(path);
    const auto finalPath = std::filesystem::path(path).replace_extension(".json");
    const auto maskPath = MakeMaskPath(path);

    // const auto maskPath =

    waitForCopyToDstBufferDone();

    auto resources = m_data->storage->getResource(m_data->hostVisibleRayDistanceBuffer);

    WriteDistanceMask(
        maskPath, *resources.rayDistanceBuffer,
        vk::Extent3D().setHeight(m_data->screenResolution.height).setWidth(m_data->screenResolution.width).setDepth(1));
    const RayDistanceMetrics distanceMetrics = calculateDistanceMetrics(resources);
    m_data->storage->returnResource(m_data->hostVisibleRayDistanceBuffer);

    std::ofstream out(finalPath.string(), std::ofstream::binary);
    const auto data =
        ImageMetrics(m_data->light, m_data->volumeInfo, m_data->controlInfo, m_data->cameraInfo.position,
                     m_data->cameraInfo.lookDir, sourcePath.filename().string(), distanceMetrics, m_data->terrainName,
                     m_data->volumeName, m_data->type, m_data->shapeInfo, m_data->terrainRenderingType)
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
    const auto *validMask = static_cast<const uint32_t *>(maskData);
    const size_t distanceCount = static_cast<size_t>(rayDistance.getBufferSize() / sizeof(float));
    const size_t maskCount = static_cast<size_t>(validRayMask.getBufferSize() / sizeof(uint32_t));

    if (distanceCount != maskCount)
    {
        validRayMask.unmap();
        rayDistance.unmap();
        STAR_THROW("Ray distance and valid ray mask buffers have different element counts");
    }

    std::span<const float> distanceSpan{distances, distanceCount};
    std::span<const uint32_t> validMaskSpan{validMask, maskCount};

    RayDistanceMetrics result{{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), 0},
                              {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), 0}};

    if (!distanceSpan.empty())
    {
        const int numValidRays = std::transform_reduce(validMaskSpan.begin(), validMaskSpan.end(), 0, std::plus<>(),
                                                        [](uint32_t x) -> int { return static_cast<int>(x); });

        result.excludingInvalidRays.rayCount = numValidRays;
        result.includingInvalidRays.rayCount = static_cast<int>(distanceSpan.size());

        const double includingInvalidSum = std::transform_reduce(
            std::execution::unseq, distanceSpan.begin(), distanceSpan.end(), validMaskSpan.begin(), 0.0,
            std::plus<double>(), [](const float distance, const uint32_t) { return static_cast<double>(distance); });

        const double excludingInvalidSum = std::transform_reduce(
            std::execution::unseq, distanceSpan.begin(), distanceSpan.end(), validMaskSpan.begin(), 0.0,
            std::plus<double>(), [](const float distance, const uint32_t valid) {
                return static_cast<double>(distance) * static_cast<double>(valid);
            });

        const double includingInvalidMin = std::transform_reduce(
            std::execution::unseq, distanceSpan.begin(), distanceSpan.end(), validMaskSpan.begin(),
            std::numeric_limits<double>::infinity(),
            [](const double lhs, const double rhs) { return std::min(lhs, rhs); },
            [](const float distance, const uint32_t) { return static_cast<double>(distance); });

        const double excludingInvalidMin = std::transform_reduce(
            std::execution::unseq, distanceSpan.begin(), distanceSpan.end(), validMaskSpan.begin(),
            std::numeric_limits<double>::infinity(),
            [](const double lhs, const double rhs) { return std::min(lhs, rhs); },
            [](const float distance, const uint32_t valid) {
                return valid > 0u ? static_cast<double>(distance) : std::numeric_limits<double>::infinity();
            });

        result.includingInvalidRays.average = includingInvalidSum / static_cast<double>(distanceSpan.size());
        result.excludingInvalidRays.average = static_cast<double>(numValidRays) > 0.0
                                                  ? excludingInvalidSum / static_cast<double>(numValidRays)
                                                  : std::numeric_limits<double>::quiet_NaN();
        result.includingInvalidRays.minimum = includingInvalidMin;
        result.excludingInvalidRays.minimum =
            static_cast<double>(numValidRays) > 0.0 ? excludingInvalidMin : std::numeric_limits<double>::quiet_NaN();
    }

    validRayMask.unmap();
    rayDistance.unmap();

    return result;
}

RayDistanceMetrics FileWriteFunction::calculateDistanceMetrics(
    const service::image_metric_manager::CopyDstResources &resources) const
{
    assert(m_data->storage != nullptr);

    RayDistanceMetrics distance;
    switch (m_data->type)
    {
    case (Fog::Type::sExponential): {
        const double val = CalcVisDistanceExponential(m_data->controlInfo, 0.98f);
        distance.includingInvalidRays.average = val;
        distance.excludingInvalidRays.average = val;
        distance.includingInvalidRays.minimum = val;
        distance.excludingInvalidRays.minimum = val;
        break;
    }
    case (Fog::Type::sLinear): {
        const double val = CalcVisDistanceLinear(m_data->controlInfo);
        distance.includingInvalidRays.average = val;
        distance.excludingInvalidRays.average = val;
        distance.includingInvalidRays.minimum = val;
        distance.excludingInvalidRays.minimum = val;
        break;
    }
    default:
        assert(resources.rayDistanceBuffer != nullptr && resources.rayAtCutoffDistBuffer != nullptr);
        distance = CalcVisDistanceFromRayBuffers(*resources.rayDistanceBuffer, *resources.rayAtCutoffDistBuffer);
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
