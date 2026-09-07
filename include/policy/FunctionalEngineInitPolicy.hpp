#pragma once

#include <starlight/policy/DefaultEngineInitPolicy.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <utility>

class FunctionalEngineInitPolicy : public star::policy::DefaultEngineInitPolicy
{
  public:
    using StartupDeviceRequirementsProvider = star::core::device::IStartupDeviceRequirementsProvider;

    explicit FunctionalEngineInitPolicy(std::function<std::vector<star::service::Service>(void)> addServicesFun,
                                        std::unique_ptr<StartupDeviceRequirementsProvider> startupDeviceRequirements)
        : star::policy::DefaultEngineInitPolicy(std::move(startupDeviceRequirements)),
          m_addServicesFun(std::move(addServicesFun))
    {
    }

    FunctionalEngineInitPolicy(std::function<std::vector<star::service::Service>(void)> addServicesFun,
                               int overrideDeviceIndex,
                               std::unique_ptr<StartupDeviceRequirementsProvider> startupDeviceRequirements)
        : star::policy::DefaultEngineInitPolicy(std::move(startupDeviceRequirements)),
          m_addServicesFun(std::move(addServicesFun)), m_overrideDeviceIndex(std::move(overrideDeviceIndex))
    {
    }

    virtual star::core::device::StarDevice createNewDevice(
        star::core::RenderingInstance &renderingInstance,
        std::set<star::Rendering_Device_Features> &engineRenderingDeviceFeatures) override;

  protected:
    std::function<std::vector<star::service::Service>(void)> m_addServicesFun;
    std::optional<int> m_overrideDeviceIndex{std::nullopt};

    virtual std::vector<star::service::Service> addAdditionalServices() override;

    star::service::Service CreateImageMetricManagerService() const;
};
