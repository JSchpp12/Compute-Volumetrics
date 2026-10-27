#pragma once

#include "FogType.hpp"

#include <star_common/Handle.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <span>

namespace render_system::fog
{
inline constexpr std::size_t kFogTypeCount = static_cast<std::size_t>(Fog::Type::sCount);

/// @brief Registration of the pipelines one fog type can bind: one handle per variant. Which variant is active is
/// chosen at runtime through getActiveVariant/setActiveVariant.
struct FogPipelineSet
{
    template <std::size_t NVariants> struct PipelineInfo
    {
        std::array<star::Handle, NVariants> handles{};
    };

    PipelineInfo<1> marched{};
    PipelineInfo<1> linear{};
    PipelineInfo<1> exponential{};
    PipelineInfo<1> nanoBoundingBox{};
    PipelineInfo<1> nanoSurface{};

    std::span<const star::Handle> handles(Fog::Type type) const noexcept
    {
        switch (type)
        {
        case Fog::Type::sMarched:
            return marched.handles;
        case Fog::Type::sLinear:
            return linear.handles;
        case Fog::Type::sExponential:
            return exponential.handles;
        case Fog::Type::sNanoBoundingBox:
            return nanoBoundingBox.handles;
        case Fog::Type::sNanoSurface:
            return nanoSurface.handles;
        default:
            return {};
        }
    }

    /// Index of the variant bound for `type`. External callers (GUI) use setActiveVariant to change it.
    std::uint32_t getActiveVariant(Fog::Type type) const noexcept
    {
        const auto index = static_cast<std::size_t>(type);
        assert(index < m_activeVariants.size() && "Fog type is outside the pipeline variant table");
        return m_activeVariants[index];
    }

    void setActiveVariant(Fog::Type type, std::uint32_t variantIndex)
    {
        assert(variantIndex < handles(type).size() && "Requested variant does not exist for this fog type's pipelines");
        m_activeVariants[static_cast<std::size_t>(type)] = variantIndex;
    }

    const star::Handle &activeHandle(Fog::Type type) const noexcept
    {
        const auto available = handles(type);
        const auto index = getActiveVariant(type);
        if (index < available.size())
        {
            return available[index];
        }

        // The selected variant has no recipe yet; nothing can bind it.
        static const star::Handle kNotBuilt{};
        return kNotBuilt;
    }

  private:
    std::array<std::uint32_t, kFogTypeCount> m_activeVariants{};
};
} // namespace render_system::fog
