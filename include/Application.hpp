#pragma once

#include <memory>

#include "OffscreenRenderer.hpp"
#include "StarApplication.hpp"
#include "Volume.hpp"

class Application : public star::StarApplication
{
  public:
    Application() = default;

    virtual ~Application() = default;
    virtual void init() override
    {
    }

    virtual std::shared_ptr<star::StarScene> loadScene(star::core::device::DeviceContext &context,
                                                       const uint8_t &numFramesInFlight) override;

    virtual void shutdown(star::core::device::DeviceContext &context) override;

  protected:
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
    void frameUpdate(star::core::SystemContext &context) override;

    static float PromptForFloat(const std::string &prompt, const bool &allowNegative = false);

    static int PromptForInt(const std::string &prompt);

    static float ProcessFloatInput(const bool &allowNegatives);

    static int ProcessIntInput();
};
