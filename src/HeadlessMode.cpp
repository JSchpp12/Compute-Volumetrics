#include "HeadlessMode.hpp"

#ifndef STAR_ENABLE_PRESENTATION
#include "Application.hpp"
#include "policy/EngineExitOnFlag.hpp"
#include "policy/FunctionalEngineInitPolicy.hpp"
#include "service/controller/CircleCameraController.hpp"

#include <starlight/StarEngine.hpp>
#include <starlight/policy/DefaultEngineLoopPolicy.hpp>

static FunctionalEngineInitPolicy CreateInit(std::shared_ptr<bool> doneFlag)
{
    auto fun = [doneFlag](void) -> std::vector<star::service::Service> {
        auto serv = std::vector<star::service::Service>(1);
        serv[0] = star::service::Service{CircleCameraController(doneFlag)};

        return serv;
    };
    return FunctionalEngineInitPolicy(fun);
}

int HeadlessMode::run()
{
    using loop = star::policy::DefaultEngineLoopPolicy;

    std::shared_ptr<bool> controllerSequenceDone = std::make_shared<bool>(false);
    using exit = EngineExitOnFlag;

    Application application;

    auto engine = star::StarEngine<FunctionalEngineInitPolicy, loop, exit>(CreateInit(controllerSequenceDone), loop{},
                                                                           exit{controllerSequenceDone}, application);
    engine.run();

    return 0;
}
#endif