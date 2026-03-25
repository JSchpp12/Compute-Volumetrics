#pragma once

#include "OffscreenRenderer.hpp"
#include "Volume.hpp"

#include <starlight/core/CommandBus.hpp>

class DeclareDependentPasses
{
    DeclareDependentPasses(const star::core::CommandBus &cmdBus, const Volume &volume, const OffscreenRenderer &offscreenRenderer);

    const Volume *m_volume{nullptr};
    const OffscreenRenderer *m_offscreenRenderer{nullptr};
    const star::core::CommandBus *m_cmdBus{nullptr};

  public:
    class Builder
    {
        star::common::EventBus &m_eventBus;
        const star::core::CommandBus &m_cmdBus;
        const Volume *m_volume{nullptr};
        const OffscreenRenderer *m_offscreenRenderer{nullptr};

      public:
        Builder(star::common::EventBus &eventBus, const star::core::CommandBus &cmdBus)
            : m_eventBus(eventBus), m_cmdBus(cmdBus) {};
        Builder &setVolume(const Volume &volume);
        Builder &setOffscreenRenderer(const OffscreenRenderer &offscreenRenderer);
        void build();
    };

    int operator()(const star::common::IEvent &e, bool &keepAlive);
};