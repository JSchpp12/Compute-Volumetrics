#include "policy/WindowedEngineInitPolicy.hpp"

#ifdef STAR_ENABLE_PRESENTATION
#include "service/controller/CircleCameraController.hpp"
#include "service/ImageMetricManager.hpp"

namespace policy
{
static star::service::Service CreateCameraControllerService()
{
    return star::service::Service{CircleCameraController{}};
}

WindowEngineInitPolicy::WindowEngineInitPolicy(star::windowing::WindowingContext &winContext) : m_winPolicy(winContext)
{
}

star::core::RenderingInstance WindowEngineInitPolicy::createRenderingInstance(std::string appName)
{
    return m_winPolicy.createRenderingInstance(std::move(appName));
}

star::core::device::StarDevice WindowEngineInitPolicy::createNewDevice(
    star::core::RenderingInstance &renderingInstance, std::set<star::Rendering_Features> &engineRenderingFeatures,
    std::set<star::Rendering_Device_Features> &engineRenderingDeviceFeatures)
{
    return m_winPolicy.createNewDevice(renderingInstance, engineRenderingFeatures, engineRenderingDeviceFeatures);
}

vk::Extent2D WindowEngineInitPolicy::getEngineRenderingResolution()
{
    return m_winPolicy.getEngineRenderingResolution();
}

star::common::FrameTracker::Setup WindowEngineInitPolicy::getFrameInFlightTrackingSetup(
    star::core::device::StarDevice &device)
{
    return m_winPolicy.getFrameInFlightTrackingSetup(device);
}

void WindowEngineInitPolicy::cleanup(star::core::RenderingInstance &instance)
{
    m_winPolicy.cleanup(instance);
}

void WindowEngineInitPolicy::init(uint8_t requestedNumFramesInFlight)
{
    m_winPolicy.init(requestedNumFramesInFlight);
}

std::vector<star::service::Service> WindowEngineInitPolicy::getAdditionalDeviceServices()
{
    auto services = m_winPolicy.getAdditionalDeviceServices();
    services.emplace_back(createImageMetricManagerService());
    services.emplace_back(CreateCameraControllerService());
    return services;
}

star::service::Service WindowEngineInitPolicy::createImageMetricManagerService() const
{
    return star::service::Service{ImageMetricManager()};
}
} // namespace policy

#endif