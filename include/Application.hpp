#pragma once

#include <memory>

#include "Interactivity.hpp"
#include "OffscreenRenderer.hpp"
#include "StarApplication.hpp"
#include "Volume.hpp"

#include <star_windowing/BasicCamera.hpp>
#include <star_windowing/WindowingContext.hpp>

class Application : public star::StarApplication
{
  public:
    explicit Application(star::windowing::WindowingContext *winContext)
        :m_winContext(winContext)
    {
    }

    void init() override
    {
    }

    std::shared_ptr<star::StarScene> loadScene(star::core::device::DeviceContext &context,
                                               const uint8_t &numFramesInFlight) override;

  private:
    star::windowing::WindowingContext *m_winContext = nullptr;
    std::shared_ptr<star::StarScene> m_mainScene = nullptr;

    star::StarObjectInstance *testObject = nullptr;
    std::shared_ptr<Volume> m_volume = nullptr;
    std::shared_ptr<std::vector<star::Light>> m_mainLight;
    std::vector<star::Handle> m_screenshotRegistrations;
    bool m_triggerScreenshot = false;
    bool m_flipScreenshotState = false;

    // void onKeyRelease(int key, int scancode, int mods) override;
    // void onMouseMovement(double xpos, double ypos) override;
    // void onMouseButtonAction(int button, int action, int mods) override;
    // void onScroll(double xoffset, double yoffset) override;
    void frameUpdate(star::core::SystemContext &context, const uint8_t &frameInFlightIndex) override;

    static float PromptForFloat(const std::string &prompt, const bool &allowNegative = false);

    static int PromptForInt(const std::string &prompt);

    static float ProcessFloatInput(const bool &allowNegatives);

    static int ProcessIntInput();

    static OffscreenRenderer CreateOffscreenRenderer(star::core::device::DeviceContext &context,
                                                     const uint8_t &numFramesInFlight,
                                                     std::shared_ptr<star::windowing::BasicCamera> camera,
                                                     std::shared_ptr<std::vector<star::Light>> mainLight);

    void triggerScreenshot(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);
};
