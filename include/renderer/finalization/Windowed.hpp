#pragma once

#ifdef STAR_ENABLE_PRESENTATION

#include "IFinalizationRenderer.hpp"

#include <star_common/IDeviceContext.hpp>
#include <star_windowing/SwapChainRenderer.hpp>

#include <vector>

namespace renderer::finalization
{
class Windowed : public star::windowing::SwapChainRenderer, public IFinalizationRenderer
{
  public:
    Windowed() = default;
    Windowed(star::windowing::WindowingContext *winContext, vk::SwapchainKHR swapChain,
             star::core::device::DeviceContext &context, std::vector<std::shared_ptr<star::StarObject>> objects,
             std::shared_ptr<std::vector<star::Light>> lights, std::shared_ptr<star::StarCamera> camera);

    Windowed(star::windowing::WindowingContext *winContext, vk::SwapchainKHR swapchain,
             star::core::device::DeviceContext &context, std::vector<std::shared_ptr<star::StarObject>> objects,
             std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightData,
             std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightListData,
             std::shared_ptr<star::ManagerController::RenderResource::Buffer> cameraData);

    virtual const star::Handle &getTimelineSemaphore(size_t index) const override
    {
        return m_timelineSemaphores[index];
    }

    virtual void prepRender(star::common::IDeviceContext &device) override;

    virtual const star::Handle &getCommandBuffer() const override
    {
        return this->star::windowing::SwapChainRenderer::getCommandBuffer();
    }

  private:
    std::vector<star::Handle> m_timelineSemaphores;
};
} // namespace renderer::finalization

#endif