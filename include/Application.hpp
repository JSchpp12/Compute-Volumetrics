#pragma once

#include "OffscreenRenderPhaseProvider.hpp"
#include "StarApplication.hpp"
#include "Volume.hpp"
#include "command/sim_controller/TriggerUpdate.hpp"
#include "loader/SceneDescription.hpp"
#include <starlight/core/renderer/IRenderPhaseProvider.hpp>

#include <functional>
#include <memory>
#include <vector>

class Application : public star::StarApplication
{
  public:
    struct VolumeRenderingOptions
    {
        bool enableCutoffHighlighting{false};
    };
    using LoaderFn = std::function<loader::SceneDescription(
        star::core::device::DeviceContext &, const std::filesystem::path &, const std::filesystem::path &)>;

    Application(LoaderFn objectLoader, std::string terrainPath, std::string volumeName,
                VolumeRenderingOptions volumeOptions);
    virtual ~Application() = default;

    virtual void init() override
    {
    }

    std::shared_ptr<star::StarScene> loadScene(star::core::device::DeviceContext &context) override;

    virtual void shutdown(star::core::device::DeviceContext &context) override;

  protected:
    struct DebugCubeInfo
    {
        std::shared_ptr<star::StarObject> debugCube;
        uint8_t numUniqueCubes{0};
    };

    LoaderFn m_loaderFn;
    std::filesystem::path m_imageOutputDir;
    std::string m_terrainDir;
    std::string m_volumeName;
    std::vector<star::Handle> m_screenshotRegistrations;
    std::optional<DebugCubeInfo> m_debugCubeInfo{std::nullopt};
    std::shared_ptr<star::StarScene> m_mainScene = nullptr;
    std::shared_ptr<Volume> m_volume;
    star::Handle m_offscreenPhaseHandle;
    std::shared_ptr<star::core::renderer::FrameData> m_offscreenFrameData;
    std::shared_ptr<std::vector<star::Light>> m_mainLight;
    std::shared_ptr<star::StarObject> m_shadowTerrain;
    star::Handle m_finalizationPhaseHandle;
    star::Handle m_volumePhaseHandle;
    star::Handle m_volumeShadowPhaseHandle;
    star::Handle m_terrainShadowPhaseHandle;
    VolumeRenderingOptions m_volumeOptions;

    bool m_flipScreenshotState = false;

    void submitPasses(star::core::device::DeviceContext &context);

    virtual void initImageOutputDir(star::core::CommandBus &bus);

    void frameUpdate(star::core::SystemContext &context) override;

    void placeDebugCubes(const glm::vec3 &direction, const glm::vec3 &startPosition);

    void setHeadlessServiceOutputDir(star::core::device::DeviceContext &context) const;

    virtual std::shared_ptr<star::StarCamera> createMainCamera(star::core::device::DeviceContext &context);

    virtual std::unique_ptr<star::core::renderer::IRenderPhaseProvider> createMainRenderer(
        star::core::device::DeviceContext &context, std::vector<std::shared_ptr<star::StarObject>> objects,
        std::shared_ptr<star::StarCamera> camera);

    virtual star::Handle getFinalizationCommandBuffer();

    virtual void triggerImageRecord(star::core::device::DeviceContext &context,
                                    const star::common::FrameTracker &frameTracker,
                                    const std::string &targetImageFileName);

    virtual void initListeners(star::core::device::DeviceContext &context) {};

    static bool CheckIfControllerIsDone(star::core::CommandBus &cmd);

    static sim_controller::UpdateStatus TriggerSimUpdate(star::core::CommandBus &cmd, Volume &volume,
                                                         star::StarCamera &camera);

    static float PromptForFloat(const std::string &prompt, const bool &allowNegative = false);

    static int PromptForInt(const std::string &prompt);

    static float ProcessFloatInput(const bool &allowNegatives);

    static int ProcessIntInput();

    std::vector<std::shared_ptr<star::StarObject>> parseSceneObjects(star::core::device::DeviceContext &context,
                                                                     std::shared_ptr<star::StarCamera> camera,
                                                                     const std::string &terrainPath);

    static star::Light CreateMainLight(glm::vec3 position);

    static void SetVolumeToCamera(Volume &volume, star::StarCamera &camera);
};
