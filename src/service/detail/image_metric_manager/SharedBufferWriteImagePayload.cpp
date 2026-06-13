#include "service/detail/image_metric_manager/SharedBufferWriteImagePayload.hpp"

#include <starlight/job/tasks/actions/WriteTiffImageAction.hpp>

namespace service::image_metric_manager
{
void SharedBufferWriteImagePayload::operator()()
{
    star::core::logging::info("Beginning file write - " + path); 

    bufferHandle->waitForCopyToDstBufferDone();
    bufferHandle->ensureMapped();

    if (imageFormat == vk::Format::eR32Sfloat)
    {
        star::job::tasks::actions::WriteTiffImageAction writer{
            .imageExtent = bufferHandle->getImageExtent(),
            .imageFormat = vk::Format::eR32Sfloat,
            .path = path,
            .dataSource = star::job::tasks::actions::RawFloatSource{bufferHandle->getMappedRayDistanceData()}};
        writer();
    }
    else
    {
        STAR_THROW("Unsupported image format for shared buffer write");
    }

    star::core::logging::info("Finished file write - " + path); 
}
} // namespace service::image_metric_manager