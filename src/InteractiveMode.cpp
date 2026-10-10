#include "InteractiveMode.hpp"

#ifdef STAR_ENABLE_PRESENTATION

#include "InteractiveApplication.hpp"
#include "config/AppConfigLoader.hpp"
#include "loader/SceneLoaders.hpp"
#include "policy/VolumetricsDeviceRequirementsProvider.hpp"
#include "service/ImageMetricManager.hpp"
#include "service/SimulationController.hpp"

#include <star_windowing/policy/EngineExitPolicy.hpp>
#include <star_windowing/policy/EngineInitPolicy.hpp>
#include <star_windowing/policy/EngineMainLoopPolicy.hpp>
#include <star_windowing/service/EngineServicesFactory.hpp>
#include <starlight/StarEngine.hpp>
#include <starlight/service/EngineServicesFactory.hpp>

#include <memory>
#include <utility>

int InteractiveMode::run(std::unique_ptr<config::AppConfigInfo> cfg)
{
    using win_exit = star::windowing::EngineExitPolicy;
    using win_loop = star::windowing::EngineMainLoopPolicy;

    star::windowing::WindowingContext winContext;
    auto startupDeviceRequirements = std::make_unique<VolumetricsDeviceRequirementsProvider>();

    auto engineServices = star::service::EngineServicesFactory::createDefault(false);
    star::windowing::EngineServicesFactory::addDefaultServices(engineServices, winContext);
    engineServices.add(star::service::Service{service::ImageMetricManager{}});
    engineServices.add(star::service::Service{SimulationControllerService(std::move(cfg->simControllerPath))});

    star::windowing::EngineInitPolicy windowInit =
        cfg->overrideRenderingDevice.has_value()
            ? star::windowing::EngineInitPolicy{winContext, engineServices, cfg->overrideRenderingDevice.value(),
                                                std::move(startupDeviceRequirements)}
            : star::windowing::EngineInitPolicy{winContext, engineServices, std::nullopt,
                                                std::move(startupDeviceRequirements)};
    win_loop windowLoop{winContext};
    win_exit windowExit{winContext};

    InteractiveApplication application =
        cfg->enableDistanceMarkers
            ? InteractiveApplication(&loader::DebugSceneLoader, cfg->terrainDir, cfg->volumeName, &winContext,
                                     {cfg->enableCutoffHighlighting, cfg->enableTransmittanceMapDebug,
                                      cfg->aggressiveTerrainTextureLoading},
                                     cfg->interactiveConfig)
            : InteractiveApplication(&loader::ReleaseSceneLoader, cfg->terrainDir, cfg->volumeName, &winContext,
                                     {cfg->enableCutoffHighlighting, cfg->enableTransmittanceMapDebug,
                                      cfg->aggressiveTerrainTextureLoading},
                                     cfg->interactiveConfig);

    auto engine = star::StarEngine<star::windowing::EngineInitPolicy, win_loop, win_exit>(
        std::move(windowInit), std::move(windowLoop), std::move(windowExit), application);

    config::AppConfigLoader::LogConfig(*cfg);
    cfg = nullptr;
    engine.run();

    return 0;
}

#endif
