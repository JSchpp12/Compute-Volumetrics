#include "util/CmdLine.hpp"

namespace util::CmdLine
{
std::optional<std::string> TryGetArgValue(int argc, char **argv, const std::string &arg) noexcept
{
    std::optional<std::string> parsed = std::nullopt;

    for (int i = 0; i < argc; i++)
    {
        std::string cArg = argv[i];

        if (cArg == arg && i + 1 < argc)
        {
            parsed = std::string(argv[++i]);
            break;
        }
    }

    return parsed;
}
} // namespace util::CmdLine