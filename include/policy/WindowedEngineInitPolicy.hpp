#pragma once

#ifdef STAR_ENABLE_PRESENTATION

#include <star_windowing/policy/EngineInitPolicy.hpp>

#include <memory>
#include <set>
#include <utility>

namespace policy
{
class WindowEngineInitPolicy
{
  public:
    using StartupDeviceRequirementsProvider = star::core::device::IStartupDeviceRequirementsProvider;

    WindowEngineInitPolicy(std::string controllerFilePath, star::windowing::WindowingContext &winContext,
                           std::unique_ptr<StartupDeviceRequirementsProvider> startupDeviceRequirements)
        : m_controllerFilePath(std::move(controllerFilePath)),
          m_winPolicy(winContext, std::move(startupDeviceRequirements))
    {
    }

    WindowEngineInitPolicy(std::string controllerFilePath, star::windowing::WindowingContext &winContext,
                           int overrideRenderingDeviceIndex,
                           std::unique_ptr<StartupDeviceRequirementsProvider> startupDeviceRequirements)
        : m_controllerFilePath(std::move(controllerFilePath)),
          m_winPolicy(winContext, overrideRenderingDeviceIndex, std::move(startupDeviceRequirements))
    {
    }

    star::core::RenderingInstance createRenderingInstance(std::string appName);

    star::core::device::StarDevice createNewDevice(
        star::core::RenderingInstance &renderingInstance,
        std::set<star::Rendering_Device_Features> &engineRenderingDeviceFeatures);

    vk::Extent2D getEngineRenderingResolution();

    star::common::FrameTracker::Setup getFrameInFlightTrackingSetup(star::core::device::StarDevice &device);

    void cleanup(star::core::RenderingInstance &instance);

    void init(uint8_t requestedNumFramesInFlight);

    std::vector<star::service::Service> getAdditionalDeviceServices();

  private:
    std::string m_controllerFilePath;
    star::windowing::EngineInitPolicy m_winPolicy;

    star::service::Service createImageMetricManagerService() const;
};
} // namespace policy

#endif
