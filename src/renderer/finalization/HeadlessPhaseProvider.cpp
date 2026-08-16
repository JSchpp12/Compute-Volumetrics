#include "renderer/finalization/HeadlessPhaseProvider.hpp"

#include "renderer/finalization/HeadlessPhase.hpp"

#include <starlight/event/RegisterMainGraphicsRenderer.hpp>

namespace renderer::finalization
{
HeadlessPhaseProvider::HeadlessPhaseProvider(star::core::device::DeviceContext &context,
                                             std::vector<std::shared_ptr<star::StarObject>> objects,
                                             std::shared_ptr<star::core::renderer::FrameData> frameData,
                                             vk::PipelineStageFlags waitStage)
    : star::core::renderer::HeadlessRenderPhaseProvider(context, std::move(objects), std::move(frameData), waitStage)
{
}

std::unique_ptr<star::core::renderer::RenderPhase>
HeadlessPhaseProvider::build(star::core::device::DeviceContext &context, star::core::renderer::RenderPhaseRegistry & /*phases*/)
{
    auto phase = std::make_unique<HeadlessPhase>();

    // shared base build, in place into phase's DefaultRenderPhase base subobject
    star::core::renderer::DefaultRenderPhase::Builder(context)
        .setObjects(std::move(m_objects))
        .setFrameData(m_frameData)
        .setDataRoles(star::core::renderer::roleHandle(star::core::renderer::frame_roles::Camera),
                      star::core::renderer::roleHandle(star::core::renderer::frame_roles::LightInfo),
                      star::core::renderer::roleHandle(star::core::renderer::frame_roles::LightList), m_createdFrameData)
        .setConfig(m_config)
        .buildInto(*phase);

    // headless tail-setup (timeline semaphores, scheme sizing, cmd-bus/device/image-manager lookup)
    prepareHeadlessPhase(phase.get(), context);

    // Register this finalization phase as the headless render-result write
    // service's main graphics renderer. build() runs during scene prepRender
    // (before the first StartOfNextFrame), so the service has the phase pointer
    // in time for its per-frame screenshot/write.
    context.getEventBus().emit(star::event::RegisterMainGraphicsRenderer{phase.get()});

    return phase;
}
} // namespace renderer::finalization
