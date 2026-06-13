#pragma once

#include <starlight/wrappers/graphics/StarBuffers/Buffer.hpp>
#include <starlight/wrappers/graphics/StarTextures/Texture.hpp>

namespace service::image_metric_manager
{
struct CopyDstResources
{
    const star::StarBuffers::Buffer *rayDistanceBuffer{nullptr};
    const star::StarBuffers::Buffer *rayAtCutoffDistBuffer{nullptr};
    const star::StarTextures::Texture *distanceMask{nullptr};
};
} // namespace image_metric_manager