#include "InteractiveMode.hpp"

#ifdef STAR_ENABLE_PRESENTATION
#include <star_windowing/policy/EngineExitPolicy.hpp>
#include <star_windowing/policy/EngineInitPolicy.hpp>
#include <star_windowing/policy/EngineMainLoopPolicy.hpp>

int InteractiveMode::run()
{
    using win_exit = star::windowing::EngineExitPolicy;
    using win_init = star::windowing::EngineInitPolicy;
    using win_loop = star::windowing::EngineMainLoopPolicy;

    star::windowing::WindowingContext winContext;
    win_init windowInit{winContext};
    win_loop windowLoop{winContext};
    win_exit windowExit{winContext};

    Application application{&winContext};
    auto engine = star::StarEngine<win_init, win_loop, win_exit>(std::move(windowInit), std::move(windowLoop),
                                                                 std::move(windowExit), application);

    engine.run();

    return 0;
}

#endif