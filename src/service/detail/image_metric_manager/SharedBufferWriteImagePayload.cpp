#include "service/detail/image_metric_manager/SharedBufferWriteImagePayload.hpp"

#include <algorithm>
#include <starlight/job/tasks/actions/WriteTiffImageAction.hpp>
namespace service::image_metric_manager
{

static void WriteNormalized(const SharedBufferHandle &buff, const std::string &path,
                            star::job::tasks::actions::WriteTiffImageAction::Compression compression)
{
    uint32_t width = buff.getImageExtent().width;
    uint32_t height = buff.getImageExtent().height;
    const float *floatData = buff.getMappedRayDistanceData();

    float minVal = FLT_MAX;
    float maxVal = -FLT_MAX;

    for (uint32_t i = 0; i < width * height; ++i)
    {
        minVal = std::min(minVal, floatData[i]);
        maxVal = std::max(maxVal, floatData[i]);
    }

    std::vector<float> normalized(width * height);
    float range = maxVal - minVal;

    for (uint32_t i = 0; i < width * height; ++i)
    {
        normalized[i] = (floatData[i] - minVal) / range;
    }

    star::job::tasks::actions::WriteTiffImageAction writer{
        .imageExtent = buff.getImageExtent(),
        .imageFormat = vk::Format::eR32Sfloat,
        .path = path,
        .dataSource = star::job::tasks::actions::RawFloatSource{normalized.data()},
        .compressionOption = std::move(compression)};
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

void SharedBufferWriteImagePayload::operator()()
{
    star::core::logging::info("Beginning file write - " + path);

    bufferHandle->waitForCopyToDstBufferDone();
    bufferHandle->ensureMapped();

    if (imageFormat == vk::Format::eR32Sfloat)
    {
        const float *data{nullptr};
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