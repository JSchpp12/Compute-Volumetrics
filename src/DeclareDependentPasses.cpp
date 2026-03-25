#include "DeclareDependentPasses.hpp"

#include <starlight/command/command_order/DeclareDependency.hpp>
#include <starlight/core/waiter/one_shot/GenericEvent.hpp>
#include <starlight/event/EnginePhaseComplete.hpp>

static void SubmitDependencies(const star::core::CommandBus &cmdBus, const Volume &volume,
                               const OffscreenRenderer &offscreenRenderer) noexcept
{
    cmdBus.submit(star::command_order::DeclareDependency{offscreenRenderer.getCommandBuffer(),
                                                         volume.getRenderer().getCommandBuffer()});
}

DeclareDependentPasses::DeclareDependentPasses(const star::core::CommandBus &cmdBus, const Volume &volume,
                                               const OffscreenRenderer &offscreenRenderer)
    : m_volume(&volume), m_offscreenRenderer(&offscreenRenderer), m_cmdBus(&cmdBus)
{
}

int DeclareDependentPasses::operator()(const star::common::IEvent &e, bool &keepAlive)
{
    const auto &event = static_cast<const star::event::EnginePhaseComplete &>(e);

    if (event.getPhase() == star::event::EnginePhaseComplete::Phase::load)
    {
        assert(m_cmdBus != nullptr);
        assert(m_volume != nullptr);
        assert(m_cmdBus != nullptr);

        SubmitDependencies(*m_cmdBus, *m_volume, *m_offscreenRenderer);
        keepAlive = false;
    }
    else
    {
        keepAlive = true;
    }

    return 0;
}

DeclareDependentPasses::Builder &DeclareDependentPasses::Builder::setVolume(const Volume &volume)
{
    m_volume = &volume;
    return *this;
}

DeclareDependentPasses::Builder &DeclareDependentPasses::Builder::setOffscreenRenderer(
    const OffscreenRenderer &offscreenRenderer)
{
    m_offscreenRenderer = &offscreenRenderer;
    return *this;
}

void DeclareDependentPasses::Builder::build()
{
    assert(m_volume != nullptr && m_offscreenRenderer != nullptr && "Both volume and renderer need to be provided");
    star::core::waiter::one_shot::GenericEvent<DeclareDependentPasses, star::event::EnginePhaseComplete>::Builder(
        m_eventBus)
        .setPayload(DeclareDependentPasses{m_cmdBus, *m_volume, *m_offscreenRenderer})
        .build();
}
