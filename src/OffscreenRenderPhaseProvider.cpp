#include "OffscreenRenderPhaseProvider.hpp"

#include "OffscreenRenderPhase.hpp"

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/core/device/system/event/ManagerRequest.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

#include <star_common/HandleTypeRegistry.hpp>

static std::vector<star::Handle> CreateSemaphores(star::common::EventBus &evtBus,
                                                  const star::common::FrameTracker &ft) noexcept
{
    const size_t num = static_cast<size_t>(ft.getSetup().getNumFramesInFlight());

    auto handles = std::vector<star::Handle>(num);
    for (size_t i{0}; i < handles.size(); i++)
    {
        void *r = nullptr;
        evtBus.emit(star::core::device::system::event::ManagerRequest(
            star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                star::core::device::manager::GetSemaphoreEventTypeName),
            star::core::device::manager::SemaphoreRequest{true}, handles[i], &r));

        if (r == nullptr)
        {
            STAR_THROW("Unable to create new semaphore");
        }
    }

    return handles;
}

OffscreenRenderPhaseProvider::OffscreenRenderPhaseProvider(star::core::device::DeviceContext &context,
                                                           std::vector<std::shared_ptr<star::StarObject>> objects,
                                                           std::shared_ptr<std::vector<star::Light>> lights,
                                                           std::shared_ptr<star::StarCamera> camera)
    : star::core::renderer::DefaultRenderPhaseProvider(context, std::move(lights), camera, std::move(objects))
{
    m_config.order = star::Command_Buffer_Order::before_render_pass;
    m_config.waitStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
}

std::unique_ptr<star::core::renderer::RenderPhase>
OffscreenRenderPhaseProvider::build(star::core::device::DeviceContext &device, star::core::renderer::RenderPhaseRegistry &phases)
{
    auto &c = device;
    auto phase = std::make_unique<OffscreenRenderPhase>();

    // offscreen pre-setup (mirrors OffscreenRenderer::prepRender before DefaultRenderer::prepRender)
    phase->m_cmdBus = &c.getCmdBus();
    phase->m_device = c.getDevice().getVulkanDevice();

    phase->graphicsQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(c.getEventBus(), c.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();
    phase->computeQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(c.getEventBus(), c.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tcompute)
            ->getParentQueueFamilyIndex();

    phase->firstFramePassCounter = uint32_t(c.frameTracker().getSetup().getNumFramesInFlight());

    auto cmd = star::command_order::DeclarePass(phase->getCommandBuffer(), phase->graphicsQueueFamilyIndex);
    c.begin().set(cmd).submit();

    phase->m_timelineSemaphores = CreateSemaphores(c.getEventBus(), c.frameTracker());

    // shared base prep (transfer state, groups, command buffer, targets, descriptor waiter)
    buildCore(phase.get(), c);

    return phase;
}