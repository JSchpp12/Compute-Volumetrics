#include "renderer/PipelineLayoutAssembler.hpp"

#include <starlight/core/Exceptions.hpp>

#include <algorithm>
#include <cassert>
#include <map>
#include <utility>
#include <vector>

namespace renderer
{
namespace
{
/// Number of set layouts a built StarShaderInfo contributes. Uses the public
/// raw-handle accessor (the layouts are prepped during DescriptorRecipe build).
size_t SetCount(star::StarShaderInfo *info)
{
    assert(info);
    return info->getDescriptorSetLayouts().size();
}

/// The first pipeline set this info occupies -- read from the StarShaderInfo so
/// the starting set index has a single source of truth (no caller hard-codes it).
uint32_t BaseSetOf(const star::StarShaderInfo *info)
{
    assert(info);
    return info->getBaseSet();
}
} // namespace

vk::PipelineLayout PipelineLayoutAssembler::operator()() const
{
    assert(device && "PipelineLayoutAssembler requires a device");

    // Collect primary contributions, sorted by baseSet so the contiguity check is
    // order-independent.
    std::vector<const LayoutContribution *> primaries;
    for (const auto &c : contributions)
        if (c.info && c.primary)
            primaries.push_back(&c);
    std::sort(primaries.begin(), primaries.end(),
              [](const LayoutContribution *a, const LayoutContribution *b) {
                  return BaseSetOf(a->info) < BaseSetOf(b->info);
              });

    // Record which primary StarShaderInfo / local set index owns each pipeline set
    // and collect their raw vk::DescriptorSetLayout handles in ascending set
    // order. Detects gaps (non-contiguous baseSets) and overlaps (two primaries
    // claiming one set).
    std::map<uint32_t, std::pair<star::StarShaderInfo *, size_t>> primarySourceBySet;
    std::vector<vk::DescriptorSetLayout> vkSets;
    uint32_t expected = 0;
    for (const auto *c : primaries)
    {
        const uint32_t baseSet = BaseSetOf(c->info);
        assert(baseSet == expected &&
               "non-contiguous primary pipeline sets -- shader-info baseSet values must cover 0..N-1 with no gaps");
        const auto handles = c->info->getDescriptorSetLayouts();
        for (size_t i = 0; i < handles.size(); ++i)
        {
            const bool inserted =
                primarySourceBySet.emplace(baseSet + static_cast<uint32_t>(i), std::make_pair(c->info, i)).second;
            assert(inserted && "two primary shader-info contributions claim the same pipeline set index");
        }
        expected += static_cast<uint32_t>(handles.size());
        vkSets.insert(vkSets.end(), handles.begin(), handles.end());
    }
    if (!primarySourceBySet.empty())
    {
        assert(primarySourceBySet.size() == static_cast<size_t>(expected) &&
               primarySourceBySet.rbegin()->first == expected - 1 &&
               "primary pipeline sets must be contiguous from 0 with no gaps or duplicates");
    }

    // Validate every validation-only contribution: each set it owns must match a
    // primary's set at the same pipeline index and be layout-compatible with it.
    // This is the check that ensures multiple StarShaderInfo bound against a
    // single shared pipeline layout are mutually compatible at every shared set.
    for (const auto &c : contributions)
    {
        if (!c.info || c.primary)
            continue;

        const uint32_t baseSet = BaseSetOf(c.info);
        const size_t count = SetCount(c.info);
        for (size_t i = 0; i < count; ++i)
        {
            const uint32_t set = baseSet + static_cast<uint32_t>(i);
            const auto it = primarySourceBySet.find(set);
            assert(it != primarySourceBySet.end() &&
                   "validation shader-info targets a pipeline set that no primary contribution defines");
            const auto [primaryInfo, primaryLocal] = it->second;
            assert(c.info->isSetLayoutCompatible(i, *primaryInfo, primaryLocal) &&
                   "StarShaderInfo bound against a shared pipeline layout is not compatible with the set baked "
                   "into it");
        }
    }

    // Bake the vk::PipelineLayout from the primary layouts (already ordered).
    const auto createInfo = vk::PipelineLayoutCreateInfo().setSetLayouts(vkSets).setPushConstantRanges(pushConstants);

    auto result = device->getVulkanDevice().createPipelineLayout(createInfo);
    if (!result)
        STAR_THROW("failed to create pipeline layout");

    return result;
}
} // namespace renderer