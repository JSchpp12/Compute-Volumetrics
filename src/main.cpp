#include "Application.hpp"
#include "StarEngine.hpp"

int main()
{
    auto engine = star::StarEngine();
    auto application = Application(engine.getScene());
    application.Load();
    engine.init(application);

    engine.Run();
}