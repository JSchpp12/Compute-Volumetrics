#pragma once

#include "service/detail/image_metric_manager/SharedBufferHandle.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/job/tasks/Task.hpp>
#include <starlight/job/tasks/actions/ImageDataTypes.hpp>

#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>

namespace service::image_metric_manager
{
struct SharedBufferWriteImagePayload
{
    std::shared_ptr<SharedBufferHandle> bufferHandle;
    vk::Format imageFormat;
    std::string path;

    void operator()();
};
} // namespace service::image_metric_manager