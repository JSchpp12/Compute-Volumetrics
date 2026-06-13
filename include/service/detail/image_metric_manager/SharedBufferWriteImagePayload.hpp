#pragma once

#include "service/detail/image_metric_manager/SharedBufferHandle.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/job/tasks/Task.hpp>

#include <memory>
#include <string>
#include <tiffio.h>
#include <vulkan/vulkan.hpp>

namespace service::image_metric_manager
{
struct SharedBufferWriteImagePayload
{
    std::shared_ptr<SharedBufferHandle> bufferHandle;
    vk::Extent3D imageExtent;
    vk::Format imageFormat;
    std::string path;

    void operator()()
    {
        bufferHandle->waitForCopyToDstBufferDone();
        bufferHandle->ensureMapped();

        if (imageFormat == vk::Format::eR32Sfloat)
        {
            writeTif();
        }
        else
        {
            STAR_THROW("Unsupported image format for shared buffer write");
        }
    }

  private:
    void writeTif() const
    {
        const uint32_t width = imageExtent.width;
        const uint32_t height = imageExtent.height;
        const float *floatData = bufferHandle->getMappedRayDistanceData();

        TIFF *tif = TIFFOpen(path.c_str(), "w");
        if (!tif)
        {
            STAR_THROW("Failed to open TIFF file for writing: " + path);
        }

        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 32);
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, height);

        for (uint32_t row = 0; row < height; ++row)
        {
            if (TIFFWriteScanline(tif, const_cast<void *>(static_cast<const void *>(floatData + row * width)), row,
                                  0) < 0)
            {
                TIFFClose(tif);
                STAR_THROW("TIFFWriteScanline failed at row " + std::to_string(row));
            }
        }

        TIFFClose(tif);
    }
};
} // namespace service::image_metric_manager