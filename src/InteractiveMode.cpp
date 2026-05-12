#include "InteractiveMode.hpp"

#ifdef STAR_ENABLE_PRESENTATION

#include "InteractiveApplication.hpp"
#include "policy/WindowedEngineInitPolicy.hpp"

#include <star_windowing/policy/EngineExitPolicy.hpp>
#include <star_windowing/policy/EngineMainLoopPolicy.hpp>
#include <starlight/StarEngine.hpp>

int InteractiveMode::run(std::unique_ptr<AppConfig> cfg)
{
    using win_exit = star::windowing::EngineExitPolicy;
    using win_loop = star::windowing::EngineMainLoopPolicy;

    star::windowing::WindowingContext winContext;
    policy::WindowEngineInitPolicy windowInit{std::move(cfg->simControllerPath), winContext};
    win_loop windowLoop{winContext};
    win_exit windowExit{winContext};

    InteractiveApplication application(std::move(cfg->terrainDir), std::move(cfg->volumeName), &winContext);
    auto engine = star::StarEngine<policy::WindowEngineInitPolicy, win_loop, win_exit>(
        std::move(windowInit), std::move(windowLoop), std::move(windowExit), application);

    engine.run();

    return 0;
}

#endif