#pragma once

#include "Volume.hpp"

#include <star_common/IServiceCommand.hpp>

#include <string_view>

namespace image_metrics
{
namespace trigger_capture
{
inline constexpr const char *GetTriggerCaptureCommandTypeName()
{
    return "imTc";
};
} // namespace trigger_capture

struct TriggerCapture : public star::common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return trigger_capture::GetTriggerCaptureCommandTypeName();
    }

    TriggerCapture(std::string srcImagePath, const Volume &volumeObject)
        : srcImagePath(std::move(srcImagePath)), volumeObject(volumeObject){}

    std::string srcImagePath;
    const Volume &volumeObject;
};
} // namespace image_metrics