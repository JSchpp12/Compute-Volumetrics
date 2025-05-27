#include "Application.hpp"
#include "StarEngine.hpp"

int main()
{
    auto engine = star::StarEngine();
    auto application = Application(engine.getScene());
    application.load();
    engine.init(application);

    engine.run();
}