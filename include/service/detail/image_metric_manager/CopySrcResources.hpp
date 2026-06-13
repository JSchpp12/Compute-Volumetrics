#pragma once

#include <star_common/Handle.hpp>
#include <starlight/wrappers/graphics/StarBuffers/Buffer.hpp>
namespace service::image_metric_manager
{
struct CopySrcResources
{
    star::Handle registration;
    star::StarBuffers::Buffer rayDistance;
    star::StarBuffers::Buffer rayAtCutoffDist;
};
} // namespace service::image_metric_manager