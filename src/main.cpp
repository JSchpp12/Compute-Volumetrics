#include "Application.hpp"
#include "StarEngine.hpp"

#include "ConfigFile.hpp"

int main()
{
    star::ConfigFile::load("./StarEngine.cfg");

    auto engine = star::StarEngine(std::make_unique<Application>());

    engine.run();
}