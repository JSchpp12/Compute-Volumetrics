#pragma once

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <string_view>

namespace image_metrics
{
namespace get_transfer_copy_pass
{
inline constexpr const char *GetTransferCopyPassCommandTypeName()
{
    return "imGTcp";
}

struct Reply
{
    star::Handle commandBuffer;
};
} // namespace get_transfer_copy_pass

struct GetTransferCopyPass : public star::common::IServiceCommandWithReply<get_transfer_copy_pass::Reply>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return get_transfer_copy_pass::GetTransferCopyPassCommandTypeName();
    }
};
} // namespace image_metrics