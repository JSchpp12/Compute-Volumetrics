#pragma once

namespace render_system
{
struct FogDispatchInfo
{
    uint32_t slicesPerSubmit;
    uint32_t totalNumSubmissions;
    uint32_t screenDimensions[2]{0, 0};
};
} // namespace render_system