#include "util/VolumePath.hpp"

#include <starlight/command/FileIO/GetPath.hpp>
#include <starlight/core/CommandBus.hpp>

#include <utility>

namespace util
{
namespace fs = std::filesystem;

using star::io::IOError;
using star::io::PathKind;

/// One GetPath round trip: ask the IOService to locate a single candidate, which
/// it probes as-is -> media-relative -> exe-relative.
static std::expected<fs::path, IOError> ResolveOne(const star::core::CommandBus &bus, fs::path candidate)
{
    star::command::file_io::GetPath cmd;
    cmd.setCandidate(std::move(candidate)).setKind(PathKind::directory);
    bus.submit(cmd);
    return cmd.getReply().get();
}

std::expected<fs::path, IOError> ResolveVolumeDirectory(const star::core::CommandBus &bus,
                                                        const std::string &volumeName)
{
    const fs::path name{volumeName};

    auto modern = ResolveOne(bus, name);
    if (modern)
    {
        return modern;
    }

    // The historical location, whose media probe finds <exeDir>/media/volumes/<name>.
    auto historical = ResolveOne(bus, fs::path{"volumes"} / name);
    if (historical)
    {
        return historical;
    }

    // Neither spelling exists. Report the failure of the primary spelling -- the
    // name the caller actually asked for.
    return std::unexpected(std::move(modern).error());
}
} // namespace util
