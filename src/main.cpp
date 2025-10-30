#include "Application.hpp"
#include "StarEngine.hpp"

#include "ConfigFile.hpp"

int main()
{
    star::ConfigFile::load("./StarEngine.cfg");

    auto application = Application(); 
    auto engine = star::StarEngine(application);

    engine.run();
}