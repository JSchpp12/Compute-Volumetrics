#pragma once

#include "OffscreenRenderer.hpp"
#include "StarApplication.hpp"
#include "Volume.hpp"

#include <memory>

class Application : public star::StarApplication
{
  public:
    explicit Application(std::string &&terrainPath);
    virtual ~Application() = default;

    virtual void init() override
    {
    }

    std::shared_ptr<star::StarScene> loadScene(star::core::device::DeviceContext &context,
                                                       const uint8_t &numFramesInFlight) override;

    virtual void shutdown(star::core::device::DeviceContext &context) override;

  protected:
    std::filesystem::path m_imageOutputDir;
    std::string m_terrainDir;
    std::shared_ptr<star::StarScene> m_mainScene = nullptr;
    star::StarObjectInstance *testObject = nullptr;
    std::shared_ptr<Volume> m_volume = nullptr;
    OffscreenRenderer *m_offRenderer{nullptr}; 
    std::shared_ptr<std::vector<star::Light>> m_mainLight;
    std::vector<star::Handle> m_screenshotRegistrations;
    bool m_flipScreenshotState = false;

    virtual void initImageOutputDir(star::core::CommandBus &bus);

    void frameUpdate(star::core::SystemContext &context) override;

    void setHeadlessServiceOutputDir(star::core::device::DeviceContext &context) const;

    virtual std::shared_ptr<star::StarCamera> createMainCamera(star::core::device::DeviceContext &context);

    virtual star::common::Renderer createMainRenderer(star::core::device::DeviceContext &context,
                                                           std::vector<std::shared_ptr<star::StarObject>> objects,
                                                           std::shared_ptr<star::StarCamera> camera);

    virtual void triggerImageRecord(star::core::device::DeviceContext &context,
                                    const star::common::FrameTracker &frameTracker,
                                    const std::string &targetImageFileName);

    virtual void initListeners(star::core::device::DeviceContext &context) {};

    static bool CheckIfControllerIsDone(star::core::CommandBus &cmd);

    static void TriggerSimUpdate(star::core::CommandBus &cmd, Volume &volume, star::StarCamera &camera);

    static float PromptForFloat(const std::string &prompt, const bool &allowNegative = false);

    static int PromptForInt(const std::string &prompt);

    static float ProcessFloatInput(const bool &allowNegatives);

    static int ProcessIntInput();

    static OffscreenRenderer CreateOffscreenRenderer(star::core::device::DeviceContext &context,
                                                     const uint8_t &numFramesInFlight,
                                                     std::shared_ptr<star::StarCamera> camera,
                                                     const std::string &terrainPath,
                                                     std::shared_ptr<std::vector<star::Light>> mainLight);

    static star::Light CreateMainLight(glm::vec3 position);

    static void SetVolumeToCamera(Volume &volume, star::StarCamera &camera);
};
