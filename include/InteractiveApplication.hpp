#pragma once

#ifdef STAR_ENABLE_PRESENTATION

#include "Application.hpp"
#include "service/SimulationController.hpp"

#include <star_windowing/BasicCamera.hpp>
#include <star_windowing/WindowingContext.hpp>
#include <star_windowing/policy/HandleKeyReleasePolicy.hpp>

class InteractiveApplication : public Application,
                               private star::windowing::HandleKeyReleasePolicy<InteractiveApplication>,
                               private star::windowing::HandleKeyPressPolicy<InteractiveApplication>
{
  public:
    InteractiveApplication(std::string &&terrainDir, star::windowing::WindowingContext *winContext)
        : Application(std::move(terrainDir)), star::windowing::HandleKeyReleasePolicy<InteractiveApplication>(*this),
          star::windowing::HandleKeyPressPolicy<InteractiveApplication>(*this), m_winContext(winContext)
    {
    }

    OffscreenRenderer createOffscreenRenderer(star::core::device::DeviceContext &context,
                                              const uint8_t &numFramesInFlight,
                                              std::shared_ptr<star::windowing::BasicCamera> camera,
                                              std::shared_ptr<std::vector<star::Light>> mainLight);
    virtual ~InteractiveApplication() = default;

    virtual void frameUpdate(star::core::SystemContext &context) override;

  protected:
    virtual void initListeners(star::core::device::DeviceContext &context) override;

  private:
    enum ModifyMode
    {
        movement,
        rotation
    };

    friend class star::windowing::HandleKeyReleasePolicy<InteractiveApplication>;
    friend class star::windowing::HandleKeyPressPolicy<InteractiveApplication>;
    star::windowing::WindowingContext *m_winContext = nullptr;
    ModifyMode m_mode{ModifyMode::movement};
    bool m_switchMode{false};
    bool m_actDir[3]{false, false, false};
    bool m_invAct{false};
    bool m_triggerScreenshot{true};

    void onKeyRelease(const int &key, const int &scancode, const int &mods);

    void onKeyPress(const int &key, const int &scancode, const int &mods);

    virtual void initImageOutputDir(star::core::CommandBus &bus) override; 

    virtual void triggerScreenshot(star::core::device::DeviceContext &context);

    virtual std::shared_ptr<star::StarCamera> createMainCamera(star::core::device::DeviceContext &context) override;

    virtual star::common::Renderer createMainRenderer(star::core::device::DeviceContext &context,
                                                           std::vector<std::shared_ptr<star::StarObject>> objects,
                                                           std::shared_ptr<star::StarCamera> camera) override;
};

#endif