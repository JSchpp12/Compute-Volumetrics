#include "service/detail/image_metric_manager/FileWriteFunction.hpp"

#include "service/detail/image_metric_manager/ImageMetrics.hpp"
#include "service/detail/image_metric_manager/ImageMetrics_json.hpp"

#include <starlight/core/helper/star_object/ObjectHelpers.hpp>

#include <algorithm>
#include <cstdint>
#include <execution>
#include <limits>
#include <numeric>
#include <ranges>
#include <span>

namespace service::image_metric_manager
{

FileWriteFunction::FileWriteFunction(std::shared_ptr<SharedBufferHandle> bufferHandle, const star::StarCamera &camera,
                                     const Volume &volume, star::Light light, std::string terrainName,
                                     star::terrain::CoverageInfo terrainShapeInfo,
                                     star::terrain::rendering::Type terrainRenderingType, std::string volumeName,
                                     ImageFilesInfo imageFilesInfo)
    : m_data(std::make_unique<MetricWriteData>(MetricWriteData{
          std::move(bufferHandle),
          ImageMetrics{.mainLight = std::move(light),
                       .volumeInfo = VolumeInfo{.position = volume.getInstance().getPosition(),
                                                .rotation = star::core::helper::star_object::ExtractRotationDegrees(
                                                    volume.getInstance().getRotationMat()),
                                                .scale = volume.getInstance().getScale()},
                       .controlInfo = volume.getRenderer().getFogInfo(),
                       .camPosition = camera.getPosition(),
                       .camLookDir = camera.getForwardVector(),
                       .distanceMetrics = {},
                       .terrainName = std::move(terrainName),
                       .volumeName = std::move(volumeName),
                       .type = volume.getRenderer().getFogType(),
                       .terrainShapeInfo = std::move(terrainShapeInfo),
                       .terrainRenderingType = terrainRenderingType,
                       .imageFilesInfo = std::move(imageFilesInfo)}}))
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

static double CalcMedian(std::vector<float> &elements)
{
    auto middle = elements.begin() + elements.size() / 2;
    std::ranges::nth_element(elements.begin(), middle, elements.end());

    if (elements.size() % 2 != 0)
    {
        return *middle;
    }
    else
    {
        // Even number of elements: mid is the upper-middle element.
        // We find the lower-middle element, which is the largest element
        // in the subrange before 'middle'.
        auto leftMax = std::max_element(elements.begin(), middle);
        return (*middle + *leftMax) / 2.0;
    }
}

static double CalcMedian(const std::span<const float> &distanceSpan, const std::span<const uint32_t> &validMaskSpan)
{
    auto filtered = std::views::zip(distanceSpan, validMaskSpan) |
                    std::views::filter([](const auto &pair) -> bool { return std::get<1>(pair) == 1u; }) |
                    std::views::transform([](const auto &pair) -> float { return std::get<0>(pair); });
    if (filtered.empty())
        return std::numeric_limits<double>::quiet_NaN();

    std::vector<float> filteredDistances(filtered.begin(), filtered.end());
    return CalcMedian(filteredDistances);
}

static RayVisibilityMetrics CalcVisDistanceFromMappedData(const float *distances, size_t distanceCount,
                                                          const uint32_t *validMask, size_t maskCount)
{
    if (distanceCount != maskCount)
    {
        STAR_THROW("Ray distance and valid ray mask buffers have different element counts");
    }

    std::span<const float> distanceSpan{distances, distanceCount};
    std::span<const uint32_t> validMaskSpan{validMask, maskCount};

    RayVisibilityMetrics result{{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN(), 0},
                                {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN(), 0}};

    if (!distanceSpan.empty())
    {
        const int numValidRays = std::transform_reduce(validMaskSpan.begin(), validMaskSpan.end(), 0, std::plus<>(),
                                                       [](uint32_t x) -> int { return static_cast<int>(x); });

        result.excludingInvalidRays.rayCount = numValidRays;
        result.includingInvalidRays.rayCount = static_cast<int>(distanceSpan.size());

        const double includingInvalidSum =
            std::transform_reduce(std::execution::unseq, distanceSpan.begin(), distanceSpan.end(), 0.0,
                                  std::plus<double>(), [](float d) { return static_cast<double>(d); });

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

        result.excludingInvalidRays.median = CalcMedian(distanceSpan, validMaskSpan);
        {
            std::vector<float> medianWorkingData(distanceSpan.begin(), distanceSpan.end());
            result.includingInvalidRays.median = CalcMedian(medianWorkingData);
        }

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

VisibilityDistanceInfo FileWriteFunction::calculateDistanceMetrics() const
{
    assert(m_data != nullptr);
    assert(m_data->bufferHandle != nullptr);

    switch (m_data->capturedMetricInfo.type)
    {
    case (Fog::Type::sExponential): {
        const double val = CalcVisDistanceExponential(m_data->capturedMetricInfo.controlInfo, 0.98f);
        return VisibilityDistanceInfo{.simpleDistance = val};
    }
    case (Fog::Type::sLinear): {
        const double val = CalcVisDistanceLinear(m_data->capturedMetricInfo.controlInfo);
        return VisibilityDistanceInfo{.simpleDistance = val};
    }
    default:
        return VisibilityDistanceInfo{
            .rayMetrics = CalcVisDistanceFromMappedData(m_data->bufferHandle->getMappedRayDistanceData(),
                                                        m_data->bufferHandle->getRayDistanceElementCount(),
                                                        m_data->bufferHandle->getMappedRayAtCutoffDistData(),
                                                        m_data->bufferHandle->getRayAtCutoffDistElementCount())};
    }
}

void FileWriteFunction::write(const std::filesystem::path &path)
{
    m_data->bufferHandle->waitForCopyToDstBufferDone();
    m_data->bufferHandle->ensureMapped();
    m_data->capturedMetricInfo.distanceMetrics = calculateDistanceMetrics();

    {
        std::ofstream out(path.string(), std::ofstream::binary);
        nlohmann::json jsonFile = m_data->capturedMetricInfo;
        out << std::setw(4) << jsonFile;
    }

    m_data->bufferHandle->ensureUnmapped();
}

int FileWriteFunction::operator()(const std::filesystem::path &filePath)
{
    write(filePath);
    return 0;
}

} // namespace service::image_metric_manager