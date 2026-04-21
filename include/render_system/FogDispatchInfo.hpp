#pragma once

namespace render_system
{
struct FogDispatchInfo
{
    uint32_t slicesPerSubmit{0};
    uint32_t totalNumSubmissions{0};
    uint32_t screenDimensions[2]{0, 0};
    uint32_t workgroupSize[2]{0, 0};
};
} // namespace render_system