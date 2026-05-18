#pragma once

#include <glm/glm.hpp>

namespace service::image_metric_manager
{
struct VolumeInfo
{
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};
} // namespace service::image_metric_manager