#pragma once

#ifdef STAR_ENABLE_PRESENTATION

#include <star_windowing/policy/EngineInitPolicy.hpp>

namespace policy
{
class WindowEngineInitPolicy
{
  public:
    explicit WindowEngineInitPolicy(star::windowing::WindowingContext &winContext);

    star::core::RenderingInstance createRenderingInstance(std::string appName); 

    star::core::device::StarDevice createNewDevice(star::core::RenderingInstance &renderingInstance, 
        std::set<star::Rendering_Features> &engineRenderingFeatures, 
        std::set<star::Rendering_Device_Features> &engineRenderingDeviceFeatures);

    vk::Extent2D getEngineRenderingResolution(); 

    star::common::FrameTracker::Setup getFrameInFlightTrackingSetup(star::core::device::StarDevice &device); 

    void cleanup(star::core::RenderingInstance &instance); 

    void init(uint8_t requestedNumFramesInFlight); 

    std::vector<star::service::Service> getAdditionalDeviceServices(); 
  private:
    star::windowing::EngineInitPolicy m_winPolicy;

    star::service::Service createImageMetricManagerService() const;
};
} // namespace policy

#endif