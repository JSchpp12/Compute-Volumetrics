#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <openvdb/openvdb.h>
#include <openvdb/tools/SignedFloodFill.h>

#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <memory>

#include "BasicCamera.hpp"
#include "Interactivity.hpp"
#include "OffscreenRenderer.hpp"
#include "StarApplication.hpp"
#include "Volume.hpp"

class Application : public star::StarApplication
{
  public:
    Application() = default;

    void onKeyPress(int key, int scancode, int mods) override;

    std::shared_ptr<star::StarScene> loadScene(star::core::device::DeviceContext &context,
                                               const star::StarWindow &window,
                                               const uint8_t &numFramesInFlight) override;

  private:
    std::shared_ptr<star::StarScene> m_mainScene = nullptr;
    std::shared_ptr<OffscreenRenderer> offscreenRenderer = nullptr;

    star::StarObjectInstance *testObject = nullptr;
    std::shared_ptr<Volume> m_volume = nullptr;
    std::shared_ptr<std::vector<star::Light>> m_mainLight;
    bool m_triggerScreenshot = false;

    void onKeyRelease(int key, int scancode, int mods) override;
    void onMouseMovement(double xpos, double ypos) override;
    void onMouseButtonAction(int button, int action, int mods) override;
    void onScroll(double xoffset, double yoffset) override;
    void frameUpdate(star::core::SystemContext &context, const uint8_t &frameInFlightIndex) override;

    static float PromptForFloat(const std::string &prompt, const bool &allowNegative = false);

    static int PromptForInt(const std::string &prompt);

    static float ProcessFloatInput(const bool &allowNegatives);

    static int ProcessIntInput();

    static std::shared_ptr<OffscreenRenderer> CreateOffscreenRenderer(
        star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
        std::shared_ptr<star::BasicCamera> camera, std::shared_ptr<std::vector<star::Light>> mainLight);

    void triggerScreenshot(star::core::device::DeviceContext &context);
};
