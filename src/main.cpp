#include "util/CmdLine.hpp"

#include <starlight/common/ConfigFile.hpp>

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <openvdb/openvdb.h>

#ifdef STAR_ENABLE_PRESENTATION
#include "InteractiveMode.hpp"

int runWindow(std::string &&terrainPath)
{
    try
    {
        InteractiveMode interactiveInstance{};
        return interactiveInstance.run(std::move(terrainPath));
    }
    catch (...)
    {
        std::cerr << "Fatal exception...exiting" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

#else
#include "HeadlessMode.hpp"
int runHeadless(std::string &&terrainPath)
{
    HeadlessMode headlessInstance{};
    return headlessInstance.run(std::move(terrainPath));
}

#endif


int main(int argc, char** argv)
{
    auto terrainPath = util::CmdLine::TryGetArgValue(argc, argv, "--terrain");
    if (!terrainPath.has_value())
    {
        std::cerr << "Controller path must be provided with arg '--controller'" << std::endl; 
        std::exit(EXIT_FAILURE); 
    }
    try
    {
        star::ConfigFile::load("./StarEngine.cfg");
    }
    catch (...)
    {
        std::cerr << "Failed to load config file for engine";
        std::exit(EXIT_FAILURE);
    }

    openvdb::initialize();

#ifdef STAR_ENABLE_PRESENTATION
    return runWindow(std::move(terrainPath.value()));
#else
    return runHeadless(std::move(terrainPath.value()));
#endif
}