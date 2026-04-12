#include "renderer/finalization/Windowed.hpp"

#ifdef STAR_ENABLE_PRESENTATION

namespace renderer::finalization
{

Windowed::Windowed(star::windowing::WindowingContext *winContext, vk::SwapchainKHR swapChain,
                   star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                   std::vector<std::shared_ptr<star::StarObject>> objects,
                   std::shared_ptr<std::vector<star::Light>> lights, std::shared_ptr<star::StarCamera> camera)
    : star::windowing::SwapChainRenderer(winContext, swapChain, context, numFramesInFlight, std::move(objects),
                                         std::move(lights), std::move(camera)),
      FinalizationRenderer()
{
}

Windowed::Windowed(star::windowing::WindowingContext *winContext, vk::SwapchainKHR swapchain,
                   star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                   std::vector<std::shared_ptr<star::StarObject>> objects,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightData,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightListData,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> cameraData)
    : star::windowing::SwapChainRenderer(winContext, swapchain, context, numFramesInFlight, std::move(objects),
                                         std::move(lightData), std::move(lightListData), std::move(cameraData)),
      FinalizationRenderer()
{
}
} // namespace renderer::finalization

#endif