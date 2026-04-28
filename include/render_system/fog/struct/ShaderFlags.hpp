#pragma once

namespace render_system::fog
{
enum class InitShaderFlags : uint16_t
{
    None = 0,
    EnableAabbTest = 1u << 0,
    EnableDepthtest = 1u << 1
};
enum class MarchShaderFlags : uint16_t
{
    None = 0
};

constexpr uint32_t Pack(InitShaderFlags initFlags, MarchShaderFlags marchFlags) noexcept
{
    return static_cast<uint32_t>(initFlags) | (static_cast<uint32_t>(marchFlags) << 16);
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