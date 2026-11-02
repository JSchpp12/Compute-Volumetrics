#pragma once

#include <star_common/Handle.hpp>

#include <cassert>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace render_system::fog
{
/// @brief Runtime handles for one pipeline family's variants: one pipeline per named variant, with one of them active.
///
/// Which variants exist and how each is configured comes from the policies; this only tracks the created pipelines
/// and which one is currently bound.
class PipelineVariantSet
{
  public:
    /// @brief Fixes the variant names and allocates the handle storage. The first name becomes the active variant.
    void prepare(std::span<const std::string_view> names)
    {
        assert(!names.empty() && "A pipeline variant set requires at least one variant");
        m_names.assign(names.begin(), names.end());
        m_handles.assign(names.size(), star::Handle{});
        m_active = names.front();
    }

    /// @brief Out handle for the variant at `index`. The reference stays valid until prepare() is called again.
    star::Handle &handleAt(const std::size_t index)
    {
        assert(index < m_handles.size() && "Pipeline variant index out of range");
        return m_handles[index];
    }

    void setActiveVariant(const std::string_view name)
    {
        const std::optional<std::size_t> index = indexOf(name);
        assert(index.has_value() && "Unknown pipeline variant");
        if (index)
        {
            m_active = m_names[*index];
        }
    }

    /// @brief Selects the variant at `index`. Out-of-range indices are ignored (asserted in debug builds).
    void setActiveVariant(const std::size_t index)
    {
        assert(index < m_names.size() && "Pipeline variant index out of range");
        if (index < m_names.size())
        {
            m_active = m_names[index];
        }
    }

    std::string_view activeVariant() const noexcept
    {
        return m_active;
    }

    /// @brief Index of the currently active variant, or `nullopt` when no variant has been prepared.
    std::optional<std::size_t> activeVariantIndex() const noexcept
    {
        return indexOf(m_active);
    }

    /// @brief Index of the variant registered under `name`, or `nullopt` when the name is unknown.
    std::optional<std::size_t> getVariantIndex(const std::string_view name) const noexcept
    {
        return indexOf(name);
    }

    const star::Handle &activeHandle() const noexcept
    {
        const std::optional<std::size_t> index = indexOf(m_active);

        // No variant has been prepared yet; nothing can be bound.
        static const star::Handle kNotBuilt{};
        return index ? m_handles[*index] : kNotBuilt;
    }

    std::span<const std::string_view> variantNames() const noexcept
    {
        return m_names;
    }

    std::span<const star::Handle> handles() const noexcept
    {
        return m_handles;
    }

    std::size_t size() const noexcept
    {
        return m_handles.size();
    }

  private:
    std::optional<std::size_t> indexOf(const std::string_view name) const noexcept
    {
        for (std::size_t i = 0; i < m_names.size(); i++)
        {
            if (m_names[i] == name)
            {
                return i;
            }
        }

        return std::nullopt;
    }

    std::vector<std::string_view> m_names;
    std::vector<star::Handle> m_handles;
    std::string_view m_active{};
};
} // namespace render_system::fog
