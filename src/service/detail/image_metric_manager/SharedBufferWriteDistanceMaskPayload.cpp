#include "service/detail/image_metric_manager/SharedBufferWriteDistanceMaskPayload.hpp"

#include <starlight/job/tasks/actions/WriteTiffImageAction.hpp>

#include <algorithm>

namespace service::image_metric_manager
{

static std::pair<float, float> GetMinMax(const float *floatData, const uint32_t width, const uint32_t height)
{
    const auto [minIt, maxIt] = std::minmax_element(floatData, floatData + (width * height));
    return std::make_pair(*minIt, *maxIt);
}
static void WriteNormalized(const SharedBufferHandle &buff, const std::string &path,
                            star::job::tasks::actions::WriteTiffImageAction::Compression compression)
{
    const uint32_t width = buff.getImageExtent().width;
    const uint32_t height = buff.getImageExtent().height;
    const float *floatData = buff.getMappedRayDistanceData();
    auto [minVal, maxVal] = GetMinMax(floatData, width, height);

    std::vector<uint8_t> shortenedData(width * height);
    const float range = maxVal - minVal;
    if (range != 0.0f)
    {
        std::transform(floatData, floatData + (width * height), shortenedData.begin(),
                       [minVal, range](float val) -> uint8_t {
                           return static_cast<uint8_t>(std::clamp((val - minVal) / range, 0.0f, 1.0f) * 255.0f);
                       });
    }

    star::job::tasks::actions::WriteTiffImageAction writer{
        .imageExtent = buff.getImageExtent(),
        .imageFormat = vk::Format::eR32Sfloat,
        .path = path,
        .dataSource = star::job::tasks::actions::RawUint8Source{shortenedData.data()},
        .compressionOption = std::move(compression),
        .precision = star::job::tasks::actions::WriteTiffImageAction::Precision::Uint8};
    writer();
}

static void Write(const SharedBufferHandle &buff, const std::string path,
                  star::job::tasks::actions::WriteTiffImageAction::Compression compression)
{
    star::job::tasks::actions::WriteTiffImageAction writer{
        .imageExtent = buff.getImageExtent(),
        .imageFormat = vk::Format::eR32Sfloat,
        .path = path,
        .dataSource = star::job::tasks::actions::RawFloatSource{buff.getMappedRayDistanceData()},
        .compressionOption = std::move(compression)};
    writer();
}

void SharedBufferWriteDistanceMaskPayload::operator()()
{
    star::core::logging::info("Beginning file write - " + path);

    bufferHandle->waitForCopyToDstBufferDone();
    bufferHandle->ensureMapped();

    if (imageFormat == vk::Format::eR32Sfloat)
    {
        auto compression = applyCompression ? star::job::tasks::actions::WriteTiffImageAction::Compression::lzw
                                            : star::job::tasks::actions::WriteTiffImageAction::Compression::none;

        if (normalizeFloatRanges)
        {
            WriteNormalized(*bufferHandle, path, std::move(compression));
        }
        else
        {
            Write(*bufferHandle, path, std::move(compression));
        }
    }
    else
    {
        STAR_THROW("Unsupported image format for shared buffer write");
    }

    star::core::logging::info("Finished file write - " + path);
}
} // namespace service::image_metric_manager