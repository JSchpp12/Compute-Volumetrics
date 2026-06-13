#include "service/detail/image_metric_manager/FileWriteFunction.hpp"

#include "service/detail/image_metric_manager/ImageMetrics.hpp"

#include <starlight/common/entities/Light_json.hpp>
#include <starlight/core/helper/star_object/ObjectHelpers.hpp>

#include <algorithm>
#include <cstdint>
#include <execution>
#include <limits>
#include <numeric>
#include <span>

namespace service::image_metric_manager
{

FileWriteFunction::FileWriteFunction(std::shared_ptr<SharedBufferHandle> bufferHandle, vk::Extent2D screenResolution,
                                      const star::StarCamera &camera, const Volume &volume, star::Light light,
                                      std::string terrainName, TerrainShapeInfo terrainShapeInfo,
                                      TerrainRenderingType terrainRenderingType, std::string volumeName,
                                      std::string sourceImageName, RayMaskFiles rayMaskFiles)
    : m_data(std::make_unique<MetricWriteData>(
          std::move(bufferHandle), std::move(terrainName), std::move(volumeName), std::move(sourceImageName),
          std::move(screenResolution),
          MetricWriteData::CameraInfo{camera.getPosition(), camera.getForwardVector()},
          VolumeInfo{.position = volume.getInstance().getPosition(),
                     .rotation =
                         star::core::helper::star_object::ExtractRotationDegrees(volume.getInstance().getRotationMat()),
                     .scale = volume.getInstance().getScale()},
          std::move(light), volume.getRenderer().getFogInfo(), volume.getRenderer().getFogType(),
          std::move(terrainShapeInfo), terrainRenderingType, std::move(rayMaskFiles)))
{
}

static double CalcVisDistanceExponential(const FogInfo &info, const float &cutoffValue)
{
    return -std::log(1.0 - cutoffValue) / (double)info.expFogInfo.density;
}

static double CalcVisDistanceLinear(const FogInfo &info)
{
    return info.linearInfo.farDist;
}

static RayDistanceMetrics CalcVisDistanceFromMappedData(const float *distances, size_t distanceCount,
                                                       const uint32_t *validMask, size_t maskCount)
{
    if (distanceCount != maskCount)
    {
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

    return result;
}

RayDistanceMetrics FileWriteFunction::calculateDistanceMetrics() const
{
    assert(m_data != nullptr);
    assert(m_data->bufferHandle != nullptr);

    const auto &resources = m_data->bufferHandle->getResources();

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
        distance = CalcVisDistanceFromMappedData(m_data->bufferHandle->getMappedRayDistanceData(),
                                                 m_data->bufferHandle->getRayDistanceElementCount(),
                                                 m_data->bufferHandle->getMappedRayAtCutoffDistData(),
                                                 m_data->bufferHandle->getRayAtCutoffDistElementCount());
        break;
    }

    return distance;
}

void FileWriteFunction::write(const std::filesystem::path &path) const
{
    m_data->bufferHandle->waitForCopyToDstBufferDone();
    m_data->bufferHandle->ensureMapped();
    const RayDistanceMetrics distanceMetrics = calculateDistanceMetrics();

    std::ofstream out(path.string(), std::ofstream::binary);
    const auto data =
        ImageMetrics(m_data->light, m_data->volumeInfo, m_data->controlInfo, m_data->cameraInfo.position,
                     m_data->cameraInfo.lookDir, m_data->sourceImageName, distanceMetrics, m_data->terrainName,
                     m_data->volumeName, m_data->type, m_data->shapeInfo, m_data->terrainRenderingType, m_data->rayMaskFiles)
            .toJsonDump();
    out << data;
}

int FileWriteFunction::operator()(const std::filesystem::path &filePath)
{
    write(filePath);
    return 0;
}

} // namespace service::image_metric_manager