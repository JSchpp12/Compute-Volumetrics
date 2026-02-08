#include "HeadlessMode.hpp"

#ifndef STAR_ENABLE_PRESENTATION
#include "Application.hpp"
#include "policy/EngineInitPolicy.hpp"

#include <starlight/StarEngine.hpp>
#include <starlight/policy/DefaultEngineLoopPolicy.hpp>
#include <starlight/policy/EngineExitAfterNumberOfFrames.hpp>

int HeadlessMode::run()
{
    using loop = star::policy::DefaultEngineLoopPolicy;
    using exit = star::policy::EngineExitAfterNumberOfFrames;

    Application application;
    auto engine = star::StarEngine<EngineInitPolicy, loop, exit>(EngineInitPolicy{}, loop{}, exit{1000}, application);
    engine.run();

    return 0;
}
#endif