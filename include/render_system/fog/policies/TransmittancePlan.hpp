#pragma once

#include "render_system/fog/const/PipelineVariantNames.hpp"
#include "render_system/fog/struct/TransmittanceVariant.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace render_system::fog::policies
{
/// @brief Minimum maxImageDimension2D every Vulkan implementation must support. Used to bound the noise layout when
/// the real device limit is not available while the plan is being built.
inline constexpr uint32_t kGuaranteedMaxImageDimension2D = 16384u;

/// @brief Immutable description of the precomputed transmittance resources and the pipeline variants which consume
/// them.
struct TransmittancePlan
{
    std::array<uint32_t, 3> transmittanceMapResolution;
    std::vector<TransmittanceVariant> variants;
    uint32_t maxSamples; /// Largest sample count in variants. Used to determine the size of the noise texture generated
    std::array<uint32_t, 2> noiseResolution;

    class Builder
    {
      public:
        Builder() = default;

        Builder &setTransmittanceMapResolution(std::array<uint32_t, 3> transmittanceMapResolution)
        {
            m_transmittanceMapResolution = std::move(transmittanceMapResolution);
            return *this;
        }

        /// @brief Sets the variant table: one pipeline per entry, first entry = default/active variant.
        Builder &setVariants(std::vector<TransmittanceVariant> variants)
        {
            m_variants = std::move(variants);
            return *this;
        }

        TransmittancePlan build();

      private:
        std::optional<std::array<uint32_t, 3>> m_transmittanceMapResolution{std::nullopt};
        std::optional<std::vector<TransmittanceVariant>> m_variants{std::nullopt};
    };
};

inline TransmittancePlan TransmittancePlan::Builder::build()
{
    assert(m_transmittanceMapResolution.has_value() && "The transmittance map resolution must be provided");
    const std::array<uint32_t, 3> &transmittanceMapResolution = m_transmittanceMapResolution.value();
    assert(transmittanceMapResolution[0] != 0u && transmittanceMapResolution[1] != 0u &&
           transmittanceMapResolution[2] != 0u && "The transmittance map resolution must be non-zero");

    assert(m_variants.has_value() && "The transmittance variant table must be provided");
    const std::vector<TransmittanceVariant> &variants = m_variants.value();

    assert(!variants.empty() && "The transmittance plan requires at least one pipeline variant");
    for (std::size_t i = 0; i < variants.size(); i++)
    {
        assert(!variants[i].name.empty() && "A transmittance variant needs a name");
        assert(variants[i].samples >= 1u && "A transmittance variant must march at least one sample per column");
        for (std::size_t j = i + 1; j < variants.size(); j++)
        {
            assert(variants[i].name != variants[j].name &&
                   "Transmittance variant names must be unique; the name is the variant identity");
        }
    }

    const uint32_t maxSamples =
        std::max_element(variants.begin(), variants.end(), [](const auto &lhs, const auto &rhs) {
            return lhs.samples < rhs.samples;
        })->samples;

    const std::array<uint32_t, 2> noiseResolution{transmittanceMapResolution[0] * maxSamples,
                                                  transmittanceMapResolution[1]};
    assert(noiseResolution[0] <= kGuaranteedMaxImageDimension2D &&
           "The transmittance noise texture width exceeds the image limit");

    return TransmittancePlan{transmittanceMapResolution, variants, maxSamples, noiseResolution};
}
} // namespace render_system::fog::policies
