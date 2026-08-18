#include "OffscreenRenderPhaseProvider.hpp"

#include "OffscreenRenderPhase.hpp"

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/core/device/system/event/ManagerRequest.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>
#include <starlight/core/renderer/RenderPhaseHelpers.hpp>

#include <star_common/HandleTypeRegistry.hpp>

OffscreenRenderPhaseProvider::OffscreenRenderPhaseProvider(star::core::device::DeviceContext &context,
                                                           std::vector<std::shared_ptr<star::StarObject>> objects,
                                                           std::shared_ptr<std::vector<star::Light>> lights,
                                                           std::shared_ptr<star::StarCamera> camera)
    : star::core::renderer::DefaultRenderPhaseProvider(context, std::move(lights), camera, std::move(objects))
{
    m_config.order = star::Command_Buffer_Order::before_render_pass;
    m_config.waitStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
}

std::unique_ptr<star::core::renderer::RenderPhase> OffscreenRenderPhaseProvider::build(
    star::core::device::DeviceContext &device, star::core::renderer::RenderPhaseRegistry &phases)
{
    auto &c = device;
    auto phase = std::make_unique<OffscreenRenderPhase>(c.getCmdBus(), c.getDevice().getVulkanDevice());

    // offscreen pre-setup (mirrors OffscreenRenderer::prepRender before DefaultRenderer::prepRender)
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

    phase->m_timelineSemaphores = star::core::renderer::CreateSemaphores(c.getEventBus(), c.frameTracker());

    // shared base build, in place into phase's DefaultRenderPhase base subobject
    star::core::renderer::DefaultRenderPhase::Builder(c)
        .setObjects(std::move(m_objects))
        .setFrameData(m_frameData)
        .setOwnsFrameData(m_createdFrameData)
        .setConfig(m_config)
        .buildInto(*phase);

    return phase;
}
