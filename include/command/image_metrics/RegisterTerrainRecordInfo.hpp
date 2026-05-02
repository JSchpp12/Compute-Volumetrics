#pragma once

#include <star_common/IServiceCommand.hpp>

#include <filesystem>
#include <string_view>

namespace image_metrics
{
namespace register_terrain_record_info
{
inline constexpr const char *GetUniqueName()
{
    return "imRTRcrd";
}
} // namespace register_terrain_record_info

struct RegisterTerrainRecordInfo : star::common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return register_terrain_record_info::GetUniqueName();
    }

    RegisterTerrainRecordInfo &setTerrainHeightFilePath(std::filesystem::path path)
    {
        terrainHeightFilePath = std::move(path);
        return *this;
    }

    std::filesystem::path terrainHeightFilePath;
};
} // namespace image_metrics