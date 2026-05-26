#include "renderer/finalization/Windowed.hpp"

#ifdef STAR_ENABLE_PRESENTATION

namespace renderer::finalization
{

static std::vector<star::Handle> CreateSemaphores(star::core::device::DeviceContext &context, const size_t &numToCreate)
{
    auto semaphores = std::vector<star::Handle>(numToCreate);

    for (size_t i{0}; i < (size_t)numToCreate; i++)
    {
        {
            auto request = star::core::device::manager::SemaphoreRequest(uint64_t{0});

            context.getEventBus().emit(star::core::device::system::event::ManagerRequest(
                star::common::HandleTypeRegistry::instance()
                    .getType(star::core::device::manager::GetSemaphoreEventTypeName)
                    .value(),
                std::move(request), semaphores[i]));
        }

        if (!semaphores[i].isInitialized())
        {
            STAR_THROW("failed to create semaphores for a frame");
        }
    }

    return semaphores;
}

Windowed::Windowed(star::windowing::WindowingContext *winContext, vk::SwapchainKHR swapChain,
                   star::core::device::DeviceContext &context, std::vector<std::shared_ptr<star::StarObject>> objects,
                   std::shared_ptr<std::vector<star::Light>> lights, std::shared_ptr<star::StarCamera> camera)
    : star::windowing::SwapChainRenderer(winContext, swapChain, context, std::move(objects), std::move(lights),
                                         std::move(camera))
{
}

Windowed::Windowed(star::windowing::WindowingContext *winContext, vk::SwapchainKHR swapchain,
                   star::core::device::DeviceContext &context, std::vector<std::shared_ptr<star::StarObject>> objects,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightData,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightListData,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> cameraData)
    : star::windowing::SwapChainRenderer(winContext, swapchain, context, std::move(objects), std::move(lightData),
                                         std::move(lightListData), std::move(cameraData))
{
}

void Windowed::prepRender(star::common::IDeviceContext &dctx)
{
    auto &d = static_cast<star::core::device::DeviceContext &>(dctx);
    m_timelineSemaphores = CreateSemaphores(d, d.frameTracker().getSetup().getNumUniqueTargetFramesForFinalization());
    this->star::windowing::SwapChainRenderer::prepRender(dctx);
}
} // namespace renderer::finalization

#endif