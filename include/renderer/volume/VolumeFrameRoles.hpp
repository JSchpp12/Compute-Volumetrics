#pragma once

#include <string_view>

/// Volume-specific FrameData role names.
namespace renderer::volume::frame_roles
{
constexpr std::string_view Fog = "Fog";
constexpr std::string_view InstanceModel = "InstanceModel";
constexpr std::string_view InstanceNormal = "InstanceNormal";
} // namespace renderer::volume::frame_roles