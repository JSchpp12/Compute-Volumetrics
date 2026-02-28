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

    TriggerCapture(std::string srcImagePath, const Volume &volumeObject, const star::StarCamera &camera, uint16_t type)
        : star::common::IServiceCommand(std::move(type)), srcImagePath(std::move(srcImagePath)),
          volumeObject(volumeObject), camera(camera)
    {
    }
    TriggerCapture(std::string srcImagePath, const Volume &volumeObject, const star::StarCamera &camera)
        : srcImagePath(std::move(srcImagePath)), volumeObject(volumeObject), camera(camera)
    {
    }

    std::string srcImagePath;
    const Volume &volumeObject;
    const star::StarCamera &camera;
};
} // namespace image_metrics