#include "HeadlessMode.hpp"

#ifndef STAR_ENABLE_PRESENTATION
#include "Application.hpp"
#include "config/AppConfigLoader.hpp"
#include "loader/SceneLoaders.hpp"
#include "policy/EngineExitOnFlag.hpp"
#include "policy/FunctionalEngineInitPolicy.hpp"
#include "service/SimulationController.hpp"

#include <starlight/StarEngine.hpp>
#include <starlight/policy/DefaultEngineLoopPolicy.hpp>

static FunctionalEngineInitPolicy CreateInit(std::shared_ptr<bool> doneFlag, std::string controllerFilePath,
                                             std::optional<int> forcedDeviceIndex = std::nullopt)
{
    auto fun = [doneFlag, controllerFilePath](void) -> std::vector<star::service::Service> {
        auto serv = std::vector<star::service::Service>(1);
        serv[0] = star::service::Service{SimulationControllerService(std::move(controllerFilePath), doneFlag)};

        return serv;
    };

    return forcedDeviceIndex.has_value() ? FunctionalEngineInitPolicy(fun, forcedDeviceIndex.value())
                                         : FunctionalEngineInitPolicy(fun);
}

int HeadlessMode::run(std::unique_ptr<config::AppConfigInfo> cfg)
{
    using loop = star::policy::DefaultEngineLoopPolicy;
    using exit = EngineExitOnFlag;
    std::shared_ptr<bool> controllerSequenceDone = std::make_shared<bool>(false);

    Application application =
        cfg->enableDistanceMarkers
            ? Application(&loader::DebugSceneLoader, cfg->terrainDir, cfg->volumeName, {cfg->enableCutoffHighlighting})
            : Application(&loader::ReleaseSceneLoader, cfg->terrainDir, cfg->volumeName,
                          {cfg->enableCutoffHighlighting});

    auto engine = star::StarEngine<FunctionalEngineInitPolicy, loop, exit>(
        CreateInit(controllerSequenceDone, cfg->simControllerPath, cfg->overrideRenderingDevice), loop{},
        exit{controllerSequenceDone}, application);

    config::AppConfigLoader::LogConfig(*cfg);
    cfg = nullptr;

    engine.run();

    return 0;
}
#endif