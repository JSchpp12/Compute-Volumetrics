#pragma once

#ifdef STAR_ENABLE_PRESENTATION

#include "FinalizationRenderer.hpp"

#include <star_windowing/SwapChainRenderer.hpp>

namespace renderer::finalization
{
class Windowed : public star::windowing::SwapChainRenderer, public FinalizationRenderer
{
  public:
    Windowed() = default;
    Windowed(star::windowing::WindowingContext *winContext, vk::SwapchainKHR swapChain,
             star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
             std::vector<std::shared_ptr<star::StarObject>> objects, std::shared_ptr<std::vector<star::Light>> lights,
             std::shared_ptr<star::StarCamera> camera);

    Windowed(star::windowing::WindowingContext *winContext, vk::SwapchainKHR swapchain,
             star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
             std::vector<std::shared_ptr<star::StarObject>> objects,
             std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightData,
             std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightListData,
             std::shared_ptr<star::ManagerController::RenderResource::Buffer> cameraData);

    virtual const star::Handle &getTimelineSemaphore(size_t index) const override
    {
        return getSemaphores()[index];
    }

    virtual const star::Handle &getCommandBuffer() const override
    {
        return this->star::windowing::SwapChainRenderer::getCommandBuffer();
    }
};
} // namespace renderer::finalization

#endif