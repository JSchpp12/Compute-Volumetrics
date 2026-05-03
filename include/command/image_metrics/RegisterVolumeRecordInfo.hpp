#pragma once
#pragma once

#include <star_common/IServiceCommand.hpp>

#include <string_view>

namespace image_metrics
{
namespace register_volume_record_info
{
inline constexpr const char *GetUniqueName()
{
    return "imRVRcrd";
}
} // namespace register_volume_record_info

struct RegisterVolumeRecordInfo : star::common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return register_volume_record_info::GetUniqueName();
    }

    RegisterVolumeRecordInfo &setVolumeName(std::string name)
    {
        volumeName = std::move(name);
        return *this;
    }

    std::string volumeName;
};
} // namespace image_metrics