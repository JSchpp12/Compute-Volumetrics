#pragma once

#ifdef STAR_ENABLE_PRESENTATION

#include "Application.hpp"
#include <star_windowing/WindowingContext.hpp>
#include <star_windowing/policy/HandleKeyReleasePolicy.hpp>

class InteractiveApplication : public Application,
                               private star::windowing::HandleKeyReleasePolicy<InteractiveApplication>
{
  public:
    explicit InteractiveApplication(star::windowing::WindowingContext *winContext)
        : star::windowing::HandleKeyReleasePolicy<InteractiveApplication>(*this), m_winContext(winContext)
    {
    }
    virtual ~InteractiveApplication() = default;

    virtual std::shared_ptr<star::StarScene> loadScene(star::core::device::DeviceContext &context,
                                                       const uint8_t &numFramesInFlight) override;

    void init() override
    {
    }

    virtual void frameUpdate(star::core::SystemContext &context) override;

  private:
    friend class star::windowing::HandleKeyReleasePolicy<InteractiveApplication>;
    star::windowing::WindowingContext *m_winContext = nullptr;

    void onKeyRelease(const int &key, const int &scancode, const int &mods);
    
    virtual void triggerScreenshot(star::core::device::DeviceContext &context, const star::common::FrameTracker &frameTracker);
};

#endif