#pragma once

#include <glm/glm.hpp>

namespace render_system::fog
{
struct DispatchInfo
{
    uint32_t workgroupSize[2]{0, 0};
    uint32_t localThreadGroupSize[2]{0, 0};
    uint32_t chunkOffsetPixels[2]{0, 0};
    uint32_t chunkOffset[2]{0, 0};
};
} // namespace render_system