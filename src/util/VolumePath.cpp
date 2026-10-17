#include "util/VolumePath.hpp"

#include <starlight/command/FileIO/GetPath.hpp>
#include <starlight/core/CommandBus.hpp>

#include <utility>

namespace util
{
namespace fs = std::filesystem;

namespace
{
using star::io::IOError;
using star::io::PathKind;

/// One GetPath round trip: ask the IOService to locate a single candidate, which
/// it probes as-is -> media-relative -> exe-relative.
std::expected<fs::path, IOError> resolveOne(const star::core::CommandBus &bus, fs::path candidate)
{
    auto cmd = star::command::file_io::GetPath::Builder{}
                   .setCandidate(std::move(candidate))
                   .setKind(PathKind::directory)
                   .build();
    bus.submit(cmd);
    return cmd.getReply().get();
}
} // namespace

std::expected<fs::path, IOError> ResolveVolumeDirectory(const star::core::CommandBus &bus,
                                                        const std::string &volumeName)
{
    const fs::path name{volumeName};

    auto modern = resolveOne(bus, name);
    if (modern)
    {
        return modern;
    }

    // The historical location, whose media probe finds <exeDir>/media/volumes/<name>.
    auto historical = resolveOne(bus, fs::path{"volumes"} / name);
    if (historical)
    {
        return historical;
    }

    // Neither spelling exists. Report the failure of the primary spelling -- the
    // name the caller actually asked for.
    return std::unexpected(std::move(modern).error());
}
} // namespace util
