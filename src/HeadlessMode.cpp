#include "HeadlessMode.hpp"

#ifndef STAR_ENABLE_PRESENTATION
#include "Application.hpp"
#include "config/AppConfigLoader.hpp"
#include "loader/SceneLoaders.hpp"
#include "policy/EngineExitOnFlag.hpp"
#include "service/ImageMetricManager.hpp"
#include "service/SimulationController.hpp"

#include <starlight/StarEngine.hpp>
#include <starlight/policy/DefaultEngineInitPolicy.hpp>
#include <starlight/policy/DefaultEngineLoopPolicy.hpp>
#include <starlight/service/EngineServicesFactory.hpp>

#include <memory>
#include <optional>
#include <utility>

int HeadlessMode::run(std::unique_ptr<config::AppConfigInfo> cfg)
{
    using loop = star::policy::DefaultEngineLoopPolicy;
    using exit = EngineExitOnFlag;
    std::shared_ptr<bool> controllerSequenceDone = std::make_shared<bool>(false);

    Application application = cfg->enableDistanceMarkers
                                  ? Application(&loader::DebugSceneLoader, cfg->terrainDir, cfg->volumeName,
                                                {cfg->enableCutoffHighlighting, cfg->enableTransmittanceMapDebug,
                                                 cfg->aggressiveTerrainTextureLoading, cfg->forceFlatTerrains})
                                  : Application(&loader::ReleaseSceneLoader, cfg->terrainDir, cfg->volumeName,
                                                {cfg->enableCutoffHighlighting, cfg->enableTransmittanceMapDebug,
                                                 cfg->aggressiveTerrainTextureLoading, cfg->forceFlatTerrains});

    auto engineServices = star::service::EngineServicesFactory::createDefault(true);
    engineServices.add(star::service::Service{service::ImageMetricManager{}});
    engineServices.add(
        star::service::Service{SimulationControllerService(std::move(cfg->simControllerPath), controllerSequenceDone)});

    auto init = star::policy::DefaultEngineInitPolicy{engineServices, {}, cfg->overrideRenderingDevice};

    auto engine = star::StarEngine<star::policy::DefaultEngineInitPolicy, loop, exit>(
        std::move(init), loop{}, exit{controllerSequenceDone}, application);

    config::AppConfigLoader::LogConfig(*cfg);
    cfg = nullptr;

    engine.run();

    return 0;
}
#endif
