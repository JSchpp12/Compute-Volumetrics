#include "InteractiveMode.hpp"

#ifdef STAR_ENABLE_PRESENTATION

#include "InteractiveApplication.hpp"
#include "loader/SceneLoaders.hpp"
#include "policy/WindowedEngineInitPolicy.hpp"
#include "config/AppConfigLoader.hpp"

#include <star_windowing/policy/EngineExitPolicy.hpp>
#include <star_windowing/policy/EngineMainLoopPolicy.hpp>
#include <starlight/StarEngine.hpp>

int InteractiveMode::run(std::unique_ptr<config::AppConfigInfo> cfg)
{
    using win_exit = star::windowing::EngineExitPolicy;
    using win_loop = star::windowing::EngineMainLoopPolicy;

    star::windowing::WindowingContext winContext;
    policy::WindowEngineInitPolicy windowInit =
        cfg->overrideRenderingDevice.has_value()
            ? policy::WindowEngineInitPolicy{std::move(cfg->simControllerPath), winContext,
                                             cfg->overrideRenderingDevice.value()}
            : policy::WindowEngineInitPolicy{std::move(cfg->simControllerPath), winContext};
    win_loop windowLoop{winContext};
    win_exit windowExit{winContext};

    InteractiveApplication application =
        cfg->enableDistanceMarkers
            ? InteractiveApplication(&loader::DebugSceneLoader, std::move(cfg->terrainDir), std::move(cfg->volumeName),
                                     &winContext, {cfg->enableCutoffHighlighting}, cfg->interactiveConfig)
            : InteractiveApplication(&loader::ReleaseSceneLoader, std::move(cfg->terrainDir),
                                     std::move(cfg->volumeName), &winContext, {cfg->enableCutoffHighlighting},
                                     cfg->interactiveConfig);
    auto engine = star::StarEngine<policy::WindowEngineInitPolicy, win_loop, win_exit>(
        std::move(windowInit), std::move(windowLoop), std::move(windowExit), application);

    config::AppConfigLoader::LogConfig(*cfg);
    engine.run();

    return 0;
}

#endif