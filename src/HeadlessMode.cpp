#include "HeadlessMode.hpp"

#ifndef STAR_ENABLE_PRESENTATION
#include "Application.hpp"

#include <starlight/policy/DefaultEngineInitPolicy.hpp>
#include <starlight/policy/EngineExitAfterNumberOfFrames.hpp>
#include <starlight/policy/DefaultEngineLoopPolicy.hpp>

int HeadlessMode::run()
{
    using init = star::policy::DefaultEngineInitPolicy;
    using loop = star::policy::DefaultEngineLoopPolicy;
    using exit = star::policy::EngineExitAfterNumberOfFrames;

    Application application;
    auto engine = star::StarEngine<init, loop, exit>({}, {}, {}, std::move(application));
    return 0;
}
#endif