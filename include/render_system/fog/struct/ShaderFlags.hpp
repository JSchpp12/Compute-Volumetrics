#pragma once

#include <cstdint>
#include <string_view>

namespace render_system::fog
{
enum class InitShaderFlags : uint8_t
{
    None = 0,
    EnableAabbTest = 1u << 0,
    EnableDepthtest = 1u << 1,
    EnableColorOutput = 1u << 2,
    EnableShadowDepthTest = 1u << 3
};
enum class MarchShaderFlags : uint16_t
{
    None = 0,
    EnableDebugHighlightCutoffValue = 1u << 0,
    EnableDebugHighlightShadows = 1u << 1,
    EnableDebugTransmittanceMap = 1u << 2,
    EnableDebugForceMarchCalculateTransmittance = 1u << 3
};
enum class PrecomputeLightTransmittanceShaderFlags : uint8_t
{
    None = 0,
    EnableDebugSetAreasInShadow = 1u << 0,
    EnableDebugDisableAabbRayTest = 1u << 1,
    EnableDebugSetTexelsOfInterest = 1u << 2
};

constexpr std::string_view to_string(MarchShaderFlags flag) noexcept
{
    switch (flag)
    {
    case (MarchShaderFlags::None):
        return "None";
    case (MarchShaderFlags::EnableDebugHighlightCutoffValue):
        return "EnableDebugHighlightCutoffValue";
    case (MarchShaderFlags::EnableDebugHighlightShadows):
        return "EnableDebugHighlightShadows";
    case (MarchShaderFlags::EnableDebugTransmittanceMap):
        return "EnableDebugTransmittanceMap";
    case (MarchShaderFlags::EnableDebugForceMarchCalculateTransmittance):
        return "EnableDebugForceMarchCalculateTransmittance";
    default:
        return "Unknown";
    }
}

constexpr std::string_view to_string(PrecomputeLightTransmittanceShaderFlags flag) noexcept
{
    switch (flag)
    {
    case (PrecomputeLightTransmittanceShaderFlags::None):
        return "None";
    case (PrecomputeLightTransmittanceShaderFlags::EnableDebugSetAreasInShadow):
        return "EnableDebugSetAreasInShadow";
    case (PrecomputeLightTransmittanceShaderFlags::EnableDebugDisableAabbRayTest):
        return "EnableDebugDisableAabbRayTest";
    case (PrecomputeLightTransmittanceShaderFlags::EnableDebugSetTexelsOfInterest):
        return "EnableDebugSetTexelsOfInterest";
    default:
        return "Unknown";
    }
}

constexpr uint32_t Pack(InitShaderFlags initFlags,
                        PrecomputeLightTransmittanceShaderFlags precomputeLightTransmittanceFlags,
                        MarchShaderFlags marchFlags) noexcept
{
    return static_cast<uint32_t>(initFlags) | (static_cast<uint32_t>(precomputeLightTransmittanceFlags) << 8) |
           (static_cast<uint32_t>(marchFlags) << 16);
}

constexpr PrecomputeLightTransmittanceShaderFlags GetPrecomputeLightTransmittanceShaderFlags(
    uint32_t flags) noexcept
{
    return static_cast<PrecomputeLightTransmittanceShaderFlags>((flags >> 8) & 0xffu);
}

constexpr MarchShaderFlags GetMarchShaderFlags(uint32_t flags) noexcept
{
    return static_cast<MarchShaderFlags>((flags >> 16) & 0xffffu);
}

// opt-in trait
template <typename E> struct EnableBitmaskOperators : std::false_type
{
};

template <> struct EnableBitmaskOperators<InitShaderFlags> : std::true_type
{
};

template <> struct EnableBitmaskOperators<MarchShaderFlags> : std::true_type
{
};

template <> struct EnableBitmaskOperators<PrecomputeLightTransmittanceShaderFlags> : std::true_type
{
};

template <typename E> inline constexpr bool EnableBitmaskOperators_v = EnableBitmaskOperators<E>::value;

// same-type operator|
template <typename E> constexpr std::enable_if_t<EnableBitmaskOperators_v<E>, E> operator|(E lhs, E rhs) noexcept
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

// same-type operator&
template <typename E> constexpr std::enable_if_t<EnableBitmaskOperators_v<E>, E> operator&(E lhs, E rhs) noexcept
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <typename E> constexpr std::enable_if_t<EnableBitmaskOperators_v<E>, E &> operator|=(E &lhs, E rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

template <typename E> constexpr std::enable_if_t<EnableBitmaskOperators_v<E>, E &> operator&=(E &lhs, E rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}

} // namespace render_system::fog