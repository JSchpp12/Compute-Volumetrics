#pragma once

#include <functional>

#include <starlight/command/command_order/DeclareDependency.hpp>
#include <starlight/core/CommandBus.hpp>
#include <starlight/core/waiter/one_shot/GenericEvent.hpp>
#include <starlight/event/EnginePhaseComplete.hpp>

class DeclareDependentPasses
{
  public:
    using HandleProvider = std::function<star::Handle()>;

  private:
    HandleProvider m_producerHandleProvider;
    HandleProvider m_consumerHandleProvider;
    const star::core::CommandBus *m_cmdBus{nullptr};

    DeclareDependentPasses(const star::core::CommandBus &cmdBus, HandleProvider producerHandleProvider,
                           HandleProvider consumerHandleProvider)
        : m_producerHandleProvider(std::move(producerHandleProvider)),
          m_consumerHandleProvider(std::move(consumerHandleProvider)), m_cmdBus(&cmdBus)
    {
    }

  public:
    class Builder
    {
      private:
        HandleProvider m_producerHandleProvider;
        HandleProvider m_consumerHandleProvider;

        star::common::EventBus &m_eventBus;
        const star::core::CommandBus &m_cmdBus;

      public:
        Builder(star::common::EventBus &eventBus, const star::core::CommandBus &cmdBus)
            : m_eventBus(eventBus), m_cmdBus(cmdBus)
        {
        }

        Builder &setProducer(HandleProvider provider)
        {
            m_producerHandleProvider = std::move(provider);
            return *this;
        }

        Builder &setConsumer(HandleProvider provider)
        {
            m_consumerHandleProvider = std::move(provider);
            return *this;
        }

        void build()
        {
            star::core::waiter::one_shot::GenericEvent<DeclareDependentPasses,
                                                       star::event::EnginePhaseComplete>::Builder(m_eventBus)
                .setPayload(DeclareDependentPasses{m_cmdBus, std::move(m_producerHandleProvider),
                                                   std::move(m_consumerHandleProvider)})
                .build();
        }
    };

    int operator()(const star::common::IEvent &e, bool &keepAlive)
    {
        const auto &event = static_cast<const star::event::EnginePhaseComplete &>(e);

        if (event.getPhase() == star::event::EnginePhaseComplete::Phase::load)
        {
            m_cmdBus->submit(
                star::command_order::DeclareDependency{m_producerHandleProvider(), m_consumerHandleProvider()});

            keepAlive = false;
        }
        else
        {
            keepAlive = true;
        }

        return 0;
    }
};