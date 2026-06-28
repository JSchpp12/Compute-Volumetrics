#include "service/detail/image_metric_manager/SharedBufferWriteValidityMaskPayload.hpp"

#include <starlight/job/tasks/actions/WritePngMaskAction.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace service::image_metric_manager
{
void SharedBufferWriteValidityMaskPayload::operator()()
{
    star::core::logging::info("Beginning file write - " + path); 
    bufferHandle->waitForCopyToDstBufferDone();
    bufferHandle->ensureMapped();

    if (imageFormat == vk::Format::eR8Uint)
    {
        const uint32_t *srcData = bufferHandle->getMappedRayAtCutoffDistData();
        const size_t elementCount = bufferHandle->getRayAtCutoffDistElementCount();
        std::vector<uint8_t> preparedMaskData(elementCount);
        for (size_t i{0}; i < elementCount; i++)
        {
            assert((srcData[i] == 1u || srcData[i] == 0u) &&
                   "Encountered invalid data in ray validity buffer. Values should always be 0 or 1");
            preparedMaskData[i] = static_cast<uint8_t>(srcData[i] * 255);
        }

        star::job::tasks::actions::WritePngMaskAction writer{
            .imageExtent = bufferHandle->getImageExtent(),
            .imageFormat = vk::Format::eR8Uint,
            .path = path,
            .dataSource = star::job::tasks::actions::RawUint8Source{preparedMaskData.data()}};
        writer();
    }
    else
    {
        STAR_THROW("Unsupported image format for validity mask write");
    }

    bufferHandle->ensureUnmapped();
    star::core::logging::info("Finished file write - " + path); 
}
} // namespace service::image_metric_manager