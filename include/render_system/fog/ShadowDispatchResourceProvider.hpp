#pragma once

#include "render_system/fog/policies/ShadowResourceResolutionPolicy.hpp"

#include <starlight/core/MappedHandleContainer.hpp>
#include <starlight/core/renderer/DescriptorRecipe.hpp>

#include <absl/container/flat_hash_map.h>

#include <memory>

namespace star
{
class StarCamera;
class Light;
} // namespace star

namespace star::core::device
{
class DeviceContext;
} // namespace star::core::device

namespace star::core::renderer
{
class FrameData;
} // namespace star::core::renderer

namespace render_system::fog
{
class ShadowDispatchResourceProvider
{
  public:
    class AdditionalResourcesInfo
    {
      public:
        struct Info
        {
            std::array<int, 3> resolution;
        };

        AdditionalResourcesInfo(std::vector<std::pair<star::Handle, Info>> records) : m_records("stFrameRole")
        {
            for (auto record : records)
            {
                m_records.manualInsert(record.first, record.second);
            }
        }

        const Info &getInfoForType(const star::Handle &type) const
        {
            return m_records.get(type);
        }

      private:
        star::core::MappedHandleContainer<Info> m_records;
    };

    explicit ShadowDispatchResourceProvider(policies::ShadowResourceResolutionPolicy resPolicy);

    /// @brief Add required resources to the frameData for use in render phase provider for the volume
    /// @param c
    /// @param frameData
    /// @return true if success, false on failure
    AdditionalResourcesInfo addResourcesTo(star::core::device::DeviceContext &c,
                                           star::core::renderer::FrameData &frameData) const noexcept;

  private:
    policies::ShadowResourceResolutionPolicy m_resPolicy;

    std::pair<star::Handle, ShadowDispatchResourceProvider::AdditionalResourcesInfo::Info> addTransmittanceMaps(
        star::core::device::DeviceContext &c, star::core::renderer::FrameData &frameData) const noexcept;
};
} // namespace render_system::fog