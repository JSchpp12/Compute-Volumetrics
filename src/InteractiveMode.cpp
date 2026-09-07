#include "InteractiveMode.hpp"

#ifdef STAR_ENABLE_PRESENTATION

#include "InteractiveApplication.hpp"
#include "config/AppConfigLoader.hpp"
#include "loader/SceneLoaders.hpp"
#include "policy/VolumetricsDeviceRequirementsProvider.hpp"
#include "policy/WindowedEngineInitPolicy.hpp"

#include <star_windowing/policy/EngineExitPolicy.hpp>
#include <star_windowing/policy/EngineMainLoopPolicy.hpp>
#include <starlight/StarEngine.hpp>

#include <memory>
#include <utility>

int InteractiveMode::run(std::unique_ptr<config::AppConfigInfo> cfg)
{
    using win_exit = star::windowing::EngineExitPolicy;
    using win_loop = star::windowing::EngineMainLoopPolicy;

    star::windowing::WindowingContext winContext;
    auto startupDeviceRequirements = std::make_unique<VolumetricsDeviceRequirementsProvider>();

    policy::WindowEngineInitPolicy windowInit =
        cfg->overrideRenderingDevice.has_value()
            ? policy::WindowEngineInitPolicy{cfg->simControllerPath, winContext, cfg->overrideRenderingDevice.value(),
                                             std::move(startupDeviceRequirements)}
            : policy::WindowEngineInitPolicy{cfg->simControllerPath, winContext, std::move(startupDeviceRequirements)};
    win_loop windowLoop{winContext};
    win_exit windowExit{winContext};

    InteractiveApplication application =
        cfg->enableDistanceMarkers
            ? InteractiveApplication(&loader::DebugSceneLoader, cfg->terrainDir, cfg->volumeName, &winContext,
                                     {cfg->enableCutoffHighlighting}, cfg->interactiveConfig)
            : InteractiveApplication(&loader::ReleaseSceneLoader, cfg->terrainDir, cfg->volumeName, &winContext,
                                     {cfg->enableCutoffHighlighting}, cfg->interactiveConfig);

    auto engine = star::StarEngine<policy::WindowEngineInitPolicy, win_loop, win_exit>(
        std::move(windowInit), std::move(windowLoop), std::move(windowExit), application);

    config::AppConfigLoader::LogConfig(*cfg);
    cfg = nullptr;
    engine.run();

    return 0;
}

#endif