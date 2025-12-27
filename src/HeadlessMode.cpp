#include "HeadlessMode.hpp"

#ifndef STAR_ENABLE_PRESENTATION
#include <starlight/policy/DefaultEngineInitPolicy.hpp>
#include <starlight/policy/EngineExitAfterNumberOfFrames.hpp>
#include <starlight/policy/IncrementalImageEngineLoopController.hpp>

int HeadlessMode::run()
{
    return 0;
}
#endif