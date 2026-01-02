#include "HeadlessMode.hpp"

#ifndef STAR_ENABLE_PRESENTATION
#include "Application.hpp"

#include <starlight/StarEngine.hpp>
#include <starlight/policy/DefaultEngineInitPolicy.hpp>
#include <starlight/policy/DefaultEngineLoopPolicy.hpp>
#include <starlight/policy/EngineExitAfterNumberOfFrames.hpp>

int HeadlessMode::run()
{
    using init = star::policy::DefaultEngineInitPolicy;
    using loop = star::policy::DefaultEngineLoopPolicy;
    using exit = star::policy::EngineExitAfterNumberOfFrames;

    Application application;
    auto engine = star::StarEngine<init, loop, exit>(init{}, loop{}, exit{8000}, application);
    engine.run();

    return 0;
}
#endif