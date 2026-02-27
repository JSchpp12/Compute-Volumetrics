#pragma once

#include <star_common/IServiceCommandWithReply.hpp>

namespace sim_controller
{
namespace check_if_done
{
inline constexpr const char *GetTypeName()
{
    return "scCid";
}
} // namespace check_if_done

struct CheckIfDone : public star::common::IServiceCommandWithReply<bool>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return check_if_done::GetTypeName();
    }

    CheckIfDone() = default;
    CheckIfDone(uint16_t type) : star::common::IServiceCommandWithReply<bool>(std::move(type))
    {
    }
};
} // namespace sim_controller